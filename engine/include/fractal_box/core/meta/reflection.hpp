#ifndef FRACTAL_BOX_CORE_REFLECTION_REFLECTION_HPP
#define FRACTAL_BOX_CORE_REFLECTION_REFLECTION_HPP

#include <algorithm>
#include <array>
#include <type_traits>

#include "fractal_box/core/algorithm.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/interval.hpp"
#include "fractal_box/core/meta/description_types.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/meta/type_name.hpp"
#include "fractal_box/core/meta/visit_fields.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

// Description-based reflection
// ============================

template<class T>
concept c_with_inline_name = requires {
	{ T::name } -> c_implicitly_convertible_to<std::string_view>;
};

template<class T>
concept c_maybe_with_inline_name = !requires { T::name; } || c_with_inline_name<T>;

template<class T>
concept c_with_inline_display_name = requires {
	{ T::display_name } -> c_implicitly_convertible_to<std::string_view>;
};

template<class T>
concept c_maybe_with_inline_display_name = !requires { T::display_name; }
	|| c_with_inline_display_name<T>;

// TODO: Accept T::describe static function
template<class T>
concept c_has_describe = requires(const T& obj) {
	fr_describe(obj);
};

template<class T>
concept c_described_class
	= c_class<T>
	&& c_has_describe<T>
	&& requires(const T& obj) {
		{ fr_describe(obj) } -> c_class_desc;
	};

template<class T>
concept c_decomposable = (c_described_class<T> || c_record_like<T>)
	&& !c_special_reflection_type<T>;

template<class T>
concept c_reflectable = (c_described_class<T> || c_aggregate<T>)
	&& !c_special_reflection_type<T>;

template<class T>
concept c_reflectable_aggregate = c_aggregate<T> && !c_special_reflection_type<T>;

namespace detail {

template<class T>
using SelectParts = typename T::Parts;

struct HashIdxPair {
	HashDigest64 hash;
	size_t idx;
};

/// @todo TODO: Rename to Reflector
template<class T>
struct Reflection;

template<c_described_class T>
struct Reflection<T> {
private:
	using Desc = decltype(fr_describe(std::declval<T>()));
	using Parts = typename Desc::Parts;
	using Names = MpFilter<Parts, IsDescriptionName>;
	using DisplayNames = MpFilter<Parts, IsDescriptionDisplayName>;

	static_assert(mp_size<Names> <= 1zu, "Type can only have one name");
	static_assert(mp_size<DisplayNames> <= 1zu, "Type can only have one display name");

	using AttributeList = MpRepack<
		MpTransform<MpFilter<Parts, IsDescriptionAttributes>, detail::SelectParts>,
		MpConcat
	>;

public:
	static constexpr auto has_name = !mp_is_empty<Names>;

	static consteval
	auto name() noexcept -> std::string_view {
		return MpFirst<Names>::value;
	}

	static constexpr auto has_display_name = !mp_is_empty<DisplayNames>;

	static consteval
	auto display_name() noexcept -> std::string_view {
		return MpFirst<DisplayNames>::value;
	}

	using Bases = MpFlatten<MpTransform<MpFilter<Parts, IsDescriptionBases>, detail::SelectParts>>;
	using Fields = MpFilter<Parts, IsDescriptionField>;
	using Properties = MpFilter<Parts, IsDescriptionProperty>;

	template<class U>
	using IsBaseOfT = typename std::is_base_of<U, T>::type;

	static_assert(mp_all_of<Bases, IsBaseOfT>, "Declared base class is not actually a base");

	static consteval
	auto check_field_ptrs() noexcept -> bool {
		// We can't compare pointers of different types at compile time, so here is the algorithm:
		// 1. Enumerate fields into type_hash/index pairs
		// 2. Sort the pairs by their type hash
		// 3. Group the pairs into consecutive same-hash chunks
		// 4. Within each chunk, retrieve pointers by pair indexes and run `test_unique_small()`
		// TODO: Replace type hashes with indexes in the unique'd type list
		if constexpr (mp_is_empty<Fields>) {
			return true;
		}
		else {
			constexpr auto num_fields = mp_size<Fields>;
			constexpr auto pairs = []<class... Fs>(MpTypes<Fs...>) {
				auto i = 0zu;
				auto arr = SimpleArray<HashIdxPair, num_fields>{{ // 1
					{type_hash64<typename Fs::PtrType>, i++}...
				}};
				std::ranges::sort(arr, std::ranges::less{}, [](const auto& pair) { // 2
					return pair.hash;
				});
				return arr;
			}(Fields{});

			constexpr auto chunks_result = [&] { // 3
				auto arr = SimpleArray<IndexInterval, num_fields>{};
				auto chunk_idx = 0zu;
				for (auto begin_idx = 0zu; begin_idx < num_fields;) {
					auto end_idx = begin_idx;
					while (end_idx < num_fields && pairs[end_idx].hash == pairs[begin_idx].hash) {
						++end_idx;
					}
					arr[chunk_idx++] = IndexInterval::from_idxs(begin_idx, end_idx);
					begin_idx = end_idx;
				}
				return std::make_pair(arr, chunk_idx);
			}();

			bool ok = true;
			unroll<chunks_result.second>([&]<size_t ChunkIdx> { // 4
				constexpr auto chunk = chunks_result.first[ChunkIdx];
				using PtrType = typename MpAt<Fields, pairs[chunk.from()].idx>::PtrType;
				[&]<size_t... Is>(std::index_sequence<Is...>) {
					constexpr PtrType ptrs[] = {
						MpAt<Fields, pairs[chunk.from() + Is].idx>::ptr...
					};
					ok = ok && test_all_unique_small(ptrs);
				}(std::make_index_sequence<chunk.size()>{});
			});

			return ok;
		}
	}

	static consteval
	auto check_field_names() noexcept -> bool {
		if constexpr (mp_is_empty<Fields>) {
			return true;
		}
		else {
			return []<class... Fs, class... Ps>(MpTypes<Fs...>, MpTypes<Ps...>) {
				constexpr std::string_view names[sizeof...(Fs) + sizeof...(Ps)] = {
					Reflection<Fs>::name()...,
					Reflection<Ps>::name()...
				};
				return test_all_unique_small(names);
			}(Fields{}, Properties{});
		}
	}

	static_assert(check_field_ptrs(), "All field pointers must be unique");
	static_assert(check_field_names(), "All field and property names must be unique");

	using Attributes = typename MpLazyIf<mp_is_empty<AttributeList>>::template Type<MpValues<>,
		AttributeList>;
	using AttributeTypes = MpValuesToTypes<Attributes>;
	using AttributeValues = MpWrapValues<Attributes>;

	template<class A>
	static constexpr auto has_attribute = mp_contains<AttributeTypes, A>;

	template<class A>
	static consteval
	auto get_attribute() noexcept -> A {
		static_assert(mp_contains_once<AttributeTypes, A>, "Attribute must appear exactly once in"
			" attributes");
		static constexpr auto idx = mp_find<AttributeTypes, A>;
		return MpAt<AttributeValues, idx>::value;
	}

	template<class A>
	static constexpr auto attributes_of_type
		= MpVFilter<Attributes, MpVTypeOfIsFn<A>::template Type>{};

	template<class A>
	static consteval
	auto get_attributes_of_type() noexcept -> std::array<A, mp_count<AttributeTypes, A>> {
		return []<auto... Vs>(MpValues<Vs...>) {
			return std::array<A, mp_count<AttributeTypes, A>>{Vs...};
		}(attributes_of_type<A>);
	}
};

template<c_description_field Field>
struct Reflection<Field> {
private:
	using Parts = typename Field::Parts;
	using Names = MpFilter<Parts, IsDescriptionName>;
	using DisplayNames = MpFilter<Parts, IsDescriptionDisplayName>;

	static_assert(mp_size<Names> <= 1zu, "Field can only have one name");
	static_assert(mp_size<DisplayNames> <= 1zu, "Field can only have one display name");

	using AttributeList = MpRepack<
		MpTransform<MpFilter<Parts, IsDescriptionAttributes>, detail::SelectParts>,
		MpConcat
	>;

public:
	static constexpr auto ptr = Field::ptr;
	using PtrType = std::remove_cvref_t<decltype(ptr)>;
	using ClassType = MemberClassType<PtrType>;
	using FieldType = MemberType<PtrType>;

	static constexpr auto has_name = !mp_is_empty<Names>;

	static consteval
	auto name() noexcept -> std::string_view {
		if constexpr (has_name)
			return MpFirst<Names>::value;
		else
			return member_name<ptr>;
	}

	static constexpr auto has_display_name = !mp_is_empty<DisplayNames>;

	static consteval
	auto display_name() noexcept -> std::string_view {
		if constexpr (has_display_name)
			return MpFirst<DisplayNames>::value;
		else
			return name();
	}

	using Attributes = typename MpLazyIf<mp_is_empty<AttributeList>>::template Type<MpValues<>,
		AttributeList>;
	using AttributeTypes = MpValuesToTypes<Attributes>;
	using AttributeValues = MpWrapValues<Attributes>;

	template<class A>
	static constexpr auto has_attribute = mp_contains<AttributeTypes, A>;

	template<class A>
	static consteval
	auto get_attribute() noexcept -> A {
		static_assert(mp_contains_once<AttributeTypes, A>, "Attribute must appear exactly once in"
			" attributes");
		static constexpr auto idx = mp_find<AttributeTypes, A>;
		using V = MpAt<AttributeValues, idx>;
		return V::value;
	}

	template<class A>
	static constexpr auto attributes_of_type
		= MpVFilter<Attributes, MpVTypeOfIsFn<A>::template Type>{};

	template<class A>
	static consteval
	auto get_attributes_of_type() noexcept -> std::array<A, mp_count<AttributeTypes, A>> {
		return []<auto... Vs>(MpValues<Vs...>) {
			return std::array<A, mp_count<AttributeTypes, A>>{Vs...};
		}(attributes_of_type<A>);
	}
};

template<c_description_property Property>
struct Reflection<Property> {
private:
	using Parts = typename Property::Parts;
	using DisplayNames = MpFilter<Parts, IsDescriptionDisplayName>;

	static_assert(mp_size<DisplayNames> <= 1zu, "Property can only have one display name");

	using AttributeList = MpRepack<
		MpTransform<MpFilter<Parts, IsDescriptionAttributes>, detail::SelectParts>,
		MpConcat
	>;

public:
	static consteval
	auto name() noexcept -> std::string_view {
		return Property::name;
	}

	static constexpr auto has_display_name = !mp_is_empty<DisplayNames>;

	static consteval
	auto display_name() noexcept -> std::string_view {
		if constexpr (has_display_name)
			return MpFirst<DisplayNames>::value;
		else
			return Property::name;
	}

	using FieldType = typename Property::Type;

	static constexpr auto getter = Property::getter;
	static constexpr auto setter = Property::setter;

	using Attributes = typename MpLazyIf<mp_is_empty<AttributeList>>::template Type<MpValues<>,
		AttributeList>;
	using AttributeTypes = MpValuesToTypes<Attributes>;
	using AttributeValues = MpWrapValues<Attributes>;

	template<class A>
	static constexpr auto has_attribute = mp_contains<AttributeTypes, A>;

	template<class A>
	static consteval
	auto get_attribute() noexcept -> A {
		static_assert(mp_contains_once<AttributeTypes, A>, "Attribute must appear exactly once in"
			" attributes");
		static constexpr auto idx = mp_find<AttributeTypes, A>;
		using V = MpAt<AttributeValues, idx>;
		return V::value;
	}

	template<class A>
	static constexpr auto attributes_of_type
		= MpVFilter<Attributes, MpVTypeOfIsFn<A>::template Type>{};

	template<class A>
	static consteval
	auto get_attributes_of_type() noexcept -> std::array<A, mp_count<AttributeTypes, A>> {
		return []<auto... Vs>(MpValues<Vs...>) {
			return std::array<A, mp_count<AttributeTypes, A>>{Vs...};
		}(attributes_of_type<A>);
	}
};

// Aggregate reflection
// ====================

template<size_t Idx, class T>
FR_FORCE_INLINE constexpr
auto get_nth_record_field(T&& obj) noexcept -> decltype(auto) {
	return visit_record_fields(
		std::forward<T>(obj),
		[]<class... Fields>(Fields&&... fields) FR_FORCE_INLINE_L -> decltype(auto)  {
			return mp_pack_at<Idx>(std::forward<Fields>(fields)...);
		}
	);
}

template<class T, size_t Idx>
inline consteval
auto get_nth_aggregate_field_name() noexcept {
	static constexpr auto func_name = detail::pretty_func_name<
		std::addressof(get_nth_record_field<Idx>(detail::fake_object<T>()))>();
	static constexpr auto slice = func_name.substr(0zu, func_name.size()
		- MemberNameHelper2::suffix_len);
	static constexpr auto trunc_name = slice.substr(slice.find_last_of("::") + 1zu);
	static constexpr auto field_name = trunc_name.substr(trunc_name.find_last_of(".") + 1zu);

	static_assert(field_name.size() > 0zu);
	return StringLiteral<char, field_name.size()>{field_name};
}

template<class T, size_t Idx>
inline constexpr auto nth_aggregate_field_name_lit = detail::get_nth_aggregate_field_name<T, Idx>();

template<size_t Idx, StringLiteral Name, class Class, class Member>
struct AggregateField {
	static constexpr auto index = Idx;
	static constexpr auto name = Name;
	using FieldType = Member;
	using ClassType = Class;
};

template<class T>
inline constexpr auto is_aggregate_field = false;

template<size_t Idx, StringLiteral Name, class Class, class Member>
inline constexpr auto is_aggregate_field<AggregateField<Idx, Name, Class, Member>> = true;

template<class T>
using IsAggregateField = BoolC<is_aggregate_field<T>>;

template<class T>
concept c_aggregate_field = is_aggregate_field<T>;

} // namespace detail

template<detail::c_aggregate_field Field>
inline constexpr auto is_special_reflection_type<Field> = true;

namespace detail {

template<c_reflectable_aggregate T>
requires (!c_has_describe<T>)
struct Reflection<T> {
	static constexpr
	auto name() noexcept { return type_name<T>; }

	static constexpr
	auto display_name() noexcept { return clean_type_name<T>; }

	using Bases = MpTypes<>;

	/// @note NOTE: Is broken in case of reference data members
	template<size_t Idx>
	using NthAggregateFieldType
		= std::remove_reference_t<decltype(get_nth_record_field<Idx>(std::declval<T&&>()))>;

	template<size_t Idx>
	using NthField = AggregateField<
		Idx,
		nth_aggregate_field_name_lit<T, Idx>,
		T,
		NthAggregateFieldType<Idx>
	>;

	using Fields = MpTransformVtoT<MpMakeIndexSequence<num_record_fields<T>>, NthField>;
	using Properties = MpTypes<>;
	using Attributes = MpValues<>;

	template<class A>
	static constexpr auto has_attribute = false;

	template<class A>
	static consteval
	auto get_attributes() noexcept -> std::array<A, 0zu> { return {}; }
};

template<c_aggregate_field Field>
struct Reflection<Field> {
	using ClassType = typename Field::ClassType;
	using FieldType = typename Field::FieldType;

	static consteval
	auto name() noexcept -> std::string_view {
		return Field::name;
	}

	static consteval
	auto display_name() noexcept -> std::string_view {
		return Field::name;
	}

	using Attributes = MpValues<>;

	template<class A>
	static constexpr auto has_attribute = false;
};

template<class T>
inline consteval
auto refl_custom_name_impl(std::string_view fallback = type_name<T>) noexcept -> std::string_view {
	if constexpr (c_described_class<T>) {
		static_assert(!c_with_inline_name<T>, "There must be a single source of truth for name");
		if constexpr (Reflection<T>::has_name)
			return Reflection<T>::name();
		else
			return fallback;
	}
	else if constexpr (c_with_inline_name<T>) {
		return T::name;
	}
	else if constexpr (is_description_field<T>
		|| is_description_property<T>
		|| is_aggregate_field<T>
	) {
		return Reflection<T>::name();
	}
	else {
		return fallback;
	}
}

template<class T>
inline consteval
auto refl_display_name_impl() noexcept -> std::string_view {
	if constexpr (c_described_class<T>) {
		static_assert(!c_with_inline_display_name<T>,
			"There must be a single source of truth for display name");
		if constexpr (Reflection<T>::has_display_name)
			return Reflection<T>::display_name();
		else
			return refl_custom_name_impl<T>(clean_type_name<T>);
	}
	else if constexpr (c_with_inline_display_name<T>) {
		return T::display_name;
	}
	else if constexpr (is_description_field<T>
		|| is_description_property<T>
		|| is_aggregate_field<T>
	) {
		return Reflection<T>::display_name();
	}
	else {
		return clean_type_name<T>;
	}
}

template<class Field>
using TypeOfField = typename Reflection<Field>::FieldType;

template<class T>
struct ReflDecompositionImpl;

template<c_described_class T>
struct ReflDecompositionImpl<T> {
	using Type = MpTransform<typename detail::Reflection<T>::Fields, TypeOfField>;
};

template<c_tuple_like T>
requires (!c_described_class<T>)
struct ReflDecompositionImpl<T> {
	using Type = decltype([]<size_t... Is>(std::index_sequence<Is...>){
		return mp_types<std::tuple_element_t<Is, T>...>;
	}(std::make_index_sequence<std::tuple_size_v<T>>{}));
};

template<c_reflectable_aggregate T>
requires (!c_described_class<T> && !c_tuple_like<T>)
struct ReflDecompositionImpl<T> {
	using Type = decltype(visit_record_fields(std::declval<T&&>(), [](auto&&... fields) {
		return mp_types<std::remove_reference_t<decltype(fields)>...>;
	}));
};

} // namespace detail

// Additional concepts
// ===================

template<class T>
concept c_can_have_attributes =
	c_has_describe<T>
	|| c_description_field<T>
	|| c_description_property<T>
	|| c_aggregate<T>
	|| detail::c_aggregate_field<T>;

template<class T>
concept c_field_or_property
	= c_description_field<T>
	|| detail::c_aggregate_field<T>
	|| c_description_property<T>;

// Reflection API
// ==============

// Common API
// ----------

// Names
// ^^^^^

template<class T>
inline constexpr auto refl_custom_name = detail::refl_custom_name_impl<T>();

template<class T>
inline constexpr auto refl_display_name = detail::refl_display_name_impl<T>();

// ReflAllAttributes
// ^^^^^^^^^^^^^^^^^

template<c_can_have_attributes T>
using ReflAllAttributes = typename detail::Reflection<T>::Attributes;

template<c_can_have_attributes T>
inline constexpr auto refl_all_attributes = ReflAllAttributes<T>{};

// refl_has_attribute
// ^^^^^^^^^^^^^^^^^^

template<c_can_have_attributes T, class Attr>
inline constexpr auto refl_has_attribute = detail::Reflection<T>
	::template has_attribute<Attr>;

template<c_can_have_attributes T, class Attr>
using ReflHasAttribute = BoolC<refl_has_attribute<T, Attr>>;

template<class T, class Attr>
concept c_has_attribute = c_can_have_attributes<T> && refl_has_attribute<T, Attr>;

// refl_attributes
// ^^^^^^^^^^^^^^^

/// @brief `std::arary<Attr, N>` of attributes
template<c_can_have_attributes T, class Attr>
inline constexpr auto refl_attributes = detail::Reflection<T>
	::template get_attributes_of_type<Attr>();

// refl_attribute
// ^^^^^^^^^^^^^^

template<c_can_have_attributes T, class Attr>
inline constexpr auto refl_attribute = detail::Reflection<T>
	::template get_attribute<Attr>();

template<c_can_have_attributes T, class Attr>
using ReflAttribute = MpValue<refl_attribute<T, Attr>>;

// refl_attribute_or
// ^^^^^^^^^^^^^^^^^

namespace detail {

template<c_can_have_attributes T, class Attr, Attr Fallback>
inline consteval
auto get_attribute_or() noexcept -> Attr {
	if constexpr (refl_has_attribute<T, Attr>)
		return refl_attribute<T, Attr>;
	else
		return Fallback;
}

} // namespace detail

/// @brief Get the attribute of the requested type if available, otherwise return `Fallback`
/// @note If there are more than one such attribute, the expression is ill-formed
template<c_can_have_attributes T, auto Fallback>
requires (refl_attributes<T, decltype(Fallback)>.size() <= 1zu)
inline constexpr auto refl_attribute_or = detail::get_attribute_or<T, decltype(Fallback),
	Fallback>();

template<c_can_have_attributes T, auto Fallback>
using ReflAttributeOr = MpValue<refl_attribute_or<T, Fallback>>;

// Class API
// ---------

// ReflBases
// ^^^^^^^^^

template<c_reflectable T>
using ReflBases = typename detail::Reflection<T>::Bases;

template<c_reflectable T>
inline constexpr auto refl_bases = ReflBases<T>{};

// ReflFields
// ^^^^^^^^^^

template<c_reflectable T>
using ReflFields = typename detail::Reflection<T>::Fields;

template<c_reflectable T>
inline constexpr auto refl_fields = ReflFields<T>{};

// num_fields
// ^^^^^^^^^^

template<class T>
using NumFields = MpSize<ReflFields<T>>;

template<class T>
inline constexpr auto num_fields = NumFields<T>{};

// ReflProperties
// ^^^^^^^^^^^^^^

template<c_reflectable T>
using ReflProperties = typename detail::Reflection<T>::Properties;

template<c_reflectable T>
inline constexpr auto refl_properties = ReflProperties<T>{};

// ReflDecomposition
// ^^^^^^^^^^^^^^^^^

template<c_decomposable T>
using ReflDecomposition = typename detail::ReflDecompositionImpl<T>::Type;

template<class T>
inline constexpr auto refl_decomposition = ReflDecomposition<T>{};

// Field API
// ---------

// nth_field_name
// ^^^^^^^^^^^^^^

template<class T, size_t Idx>
inline constexpr auto nth_field_name = detail::MpIllegal{};

template<class T, size_t Idx>
requires (c_described_class<T>)
inline constexpr auto nth_field_name<T, Idx> = refl_custom_name<MpAt<ReflFields<T>, Idx>>;

template<class T, size_t Idx>
requires (c_aggregate<T> && !c_described_class<T>)
inline constexpr auto nth_field_name<T, Idx> = std::string_view(
	detail::nth_aggregate_field_name_lit<T, Idx>);

// refl_field_ptr
// ^^^^^^^^^^^^^^

template<c_description_field Field>
using ReflFieldPtr = MpValue<Field::ptr>;

template<c_description_field Field>
inline constexpr auto refl_field_ptr = Field::ptr;

// ReflFieldPtrType
// ^^^^^^^^^^^^^^^^

template<c_description_field Field>
using ReflFieldPtrType = typename detail::Reflection<Field>::PtrType;

template<c_description_field Field>
inline constexpr auto refl_field_ptr_type = mp_type<ReflFieldPtrType<Field>>;

// ReflFieldClassType
// ^^^^^^^^^^^^^^^^^^

template<class Field>
requires (c_description_field<Field> || detail::c_aggregate_field<Field>)
using ReflFieldClassType = typename detail::Reflection<Field>::ClassType;

template<class Field>
requires (c_description_field<Field> || detail::c_aggregate_field<Field>)
inline constexpr auto refl_field_class_type = mp_type<ReflFieldClassType<Field>>;

// ReflFieldType
// ^^^^^^^^^^^^^

template<class Field>
requires (c_description_field<Field> || detail::c_aggregate_field<Field>)
using ReflFieldType = typename detail::Reflection<Field>::FieldType;

template<class Field>
requires (c_description_field<Field> || detail::c_aggregate_field<Field>)
inline constexpr auto refl_field_type = mp_type<ReflFieldClassType<Field>>;

// Functions
// ^^^^^^^^^

template<class Field, class T>
requires ((c_description_field<Field> || detail::c_aggregate_field<Field>)
	&& std::same_as<ReflFieldClassType<Field>, std::remove_cvref_t<T>>)
FR_FORCE_INLINE constexpr
auto apply_field(T&& obj) noexcept -> decltype(auto) {
	if constexpr (is_description_field<Field>) {
		return std::forward<T>(obj).*Field::ptr;
	}
	else if constexpr (detail::is_aggregate_field<Field>) {
		return detail::get_nth_record_field<Field::index>(obj);
	}
	else {
		static_assert(false);
	}
}

template<size_t Idx, class T>
requires c_decomposable<std::remove_cvref_t<T>>
FR_FORCE_INLINE constexpr FR_FLATTEN
auto get(T&& obj) noexcept -> decltype(auto) {
	using PT = std::remove_cvref_t<T>;
	if constexpr (c_described_class<PT>) {
		using Field = MpAt<ReflFields<PT>, Idx>;
		return std::forward<T>(obj).*Field::ptr;
	}
	else if constexpr (c_tuple_like<PT>) {
		return std::get<Idx>(obj);
	}
	else if constexpr (c_aggregate<PT>) {
		return detail::get_nth_record_field<Idx>(std::forward<T>(obj));
	}
	else {
		static_assert(false);
	}
}

template<class T, class F>
requires c_decomposable<std::remove_cvref_t<T>>
FR_FORCE_INLINE constexpr
void for_each_field(T&& obj, F&& callback) {
	using PT = std::remove_cvref_t<T>;
	if constexpr (c_described_class<PT>) {
		[&]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
			(..., static_cast<void>(std::forward<F>(callback)(
				std::forward<T>(obj).*MpAt<ReflFields<PT>, Is>::ptr)));
		}(std::make_index_sequence<mp_size<ReflFields<PT>>>{});
	}
	else if constexpr (c_record_like<PT>) {
		[&]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
			(..., static_cast<void>(std::forward<F>(callback)(
				detail::get_nth_record_field<Is>(std::forward<T>(obj)))));
		}(std::make_index_sequence<mp_size<ReflFields<PT>>>{});
	}
	else {
		static_assert(false);
	}
}

template<class F, class T>
requires c_decomposable<std::remove_cvref_t<T>>
FR_FORCE_INLINE constexpr
auto visit(T&& obj, F&& f) -> decltype(auto) {
	using PT = std::remove_cvref_t<T>;
	if constexpr (c_described_class<PT>) {
		using Fields = ReflFields<PT>;
		return [&]<size_t... Is>(
			std::index_sequence<Is...>
		) FR_FORCE_INLINE_L -> decltype(auto) {
			return std::forward<F>(f)(std::forward<T>(obj).*MpAt<Fields, Is>::ptr...);
		}(std::make_index_sequence<mp_size<Fields>>{});
	}
	else if constexpr (c_record_like<PT>) {
		return visit_record_fields(std::forward<T>(obj), std::forward<F>(f));
	}
	else {
		static_assert(false);
	}
}

// Property API
// ------------

// ReflPropertyType
// ^^^^^^^^^^^^^^^^

template<c_description_property Property>
using ReflPropertyType = typename detail::Reflection<Property>::FieldType;

template<c_description_property Property>
inline constexpr auto refl_property_type = mp_type<ReflPropertyType<Property>>;

// refl_property_getter
// ^^^^^^^^^^^^^^^^^^^^

template<c_description_property Property>
static constexpr auto refl_property_getter = detail::Reflection<Property>::getter;

template<c_description_property Property>
using ReflPropertyGetter = MpValue<refl_property_getter<Property>>;

// refl_property_setter
// ^^^^^^^^^^^^^^^^^^^^

template<c_description_property Property>
static constexpr auto refl_property_setter = detail::Reflection<Property>::setter;

template<c_description_property Property>
using ReflPropertySettter = MpValue<refl_property_setter<Property>>;

// Functions
// ^^^^^^^^^

/// @todo TODO: Get field/property by name
template<c_description_property Property, class T>
FR_FORCE_INLINE constexpr
auto get_property(T&& obj) -> decltype(auto) {
	using Getter = std::remove_cvref_t<decltype(Property::getter)>;
	if constexpr (std::is_member_function_pointer_v<Getter>) {
		static_assert(std::is_same_v<std::remove_cvref_t<T>, MemberClassType<Getter>>);
		return (std::forward<T>(obj).*Property::getter)();
	}
	else {
		return Property::getter(std::forward<T>(obj));
	}
}

/// @todo TODO: Set field/property by name
template<c_description_property Property, class T, class V>
requires Property::has_setter
FR_FORCE_INLINE constexpr
void set_property(T&& obj, V&& value) {
	using Setter = std::remove_cvref_t<decltype(Property::setter)>;
	if constexpr (std::is_member_function_pointer_v<Setter>) {
		static_assert(std::is_same_v<std::remove_cvref_t<T>, MemberClassType<Setter>>);
		(std::forward<T>(obj).*Property::setter)(std::forward<V>(value));
	}
	else {
		Property::setter(std::forward<T>(obj), std::forward<V>(value));
	}
}

// Compbined field & property API
// ------------------------------

// ReflFieldOrPropertyType
// ^^^^^^^^^^^^^^^^^^^^^^^

template<c_field_or_property Child>
using ReflFieldOrPropertyType = typename detail::Reflection<Child>::FieldType;

template<c_field_or_property Child>
inline constexpr auto refl_field_or_property_type = mp_type<ReflFieldOrPropertyType<Child>>;

// ReflFieldsAndProperties
// ^^^^^^^^^^^^^^^^^^^^^^^

template<c_reflectable T>
using ReflFieldsAndProperties = MpConcat<ReflFields<T>, ReflProperties<T>>;

template<c_reflectable T>
inline constexpr auto refl_fields_and_properties = ReflFieldsAndProperties<T>{};

// Functions
// ^^^^^^^^^

template<c_field_or_property FP, class T>
FR_FORCE_INLINE constexpr
auto get_field_or_property(T&& obj) -> decltype(auto) {
	if constexpr (c_description_property<FP>)
		return get_property<FP>(std::forward<T>(obj));
	else
		return apply_field<FP>(std::forward<T>(obj));
}

template<c_field_or_property FP, class T, class V>
FR_FORCE_INLINE constexpr
void set_field_or_property(T&& obj, V&& value) {
	if constexpr (c_description_property<FP>)
		set_property<FP>(std::forward<T>(obj), std::forward<V>(value));
	else
		apply_field<FP>(std::forward<T>(obj)) = std::forward<V>(value);
}

} // namespace fr
#endif
