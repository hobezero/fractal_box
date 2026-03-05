#ifndef FRACTAL_BOX_CORE_HASHING_HASH2_HPP
#define FRACTAL_BOX_CORE_HASHING_HASH2_HPP

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/hashing/hasher_visitor_base.hpp"
#include "fractal_box/core/hashing/hashing_attributes.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/meta/reflection.hpp"

namespace fr {

namespace detail {

struct DummyHasherVisitor: public HasherVisitorBase {
	constexpr
	auto operator()(const auto&...) noexcept { }
};

} // namespace detail

template<class T>
concept c_has_custom_hash = requires(const T object, detail::DummyHasherVisitor visitor) {
	{ fr_custom_hash(object, visitor) };
};

template<class T, class Hasher>
concept c_hashable_by = requires(const T object, Hasher hasher) {
	hasher(object);
};

enum class HashableCategory: uint8_t {
	Unhashable,
	Primitive,
	Wrapper,
	Custom,
	Described,
	Enum,
	Optional,
	String,
	Array,
	Range,
	Record,
};

class Hashability {
public:
	Hashability() = default;

	FR_FORCE_INLINE constexpr
	Hashability(HashableCategory category, HashableMode mode) noexcept:
		_category{category},
		_mode{mode}
	{
		if (_category == HashableCategory::Unhashable) {
			FR_ASSERT_MSG(_mode == HashableMode::None, "Category/mode mismatch");
		}
	}

	friend
	auto operator==(Hashability, Hashability) -> bool = default;

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept {
		return _mode != HashableMode::None;
	}

	FR_FORCE_INLINE constexpr
	auto category() const noexcept -> HashableCategory { return _category; }

	FR_FORCE_INLINE constexpr
	auto mode() const noexcept -> HashableMode { return _mode; }

private:
	HashableCategory _category = HashableCategory::Unhashable;
	HashableMode _mode = HashableMode::None;
};

template<class T>
inline consteval
auto get_hashability() noexcept -> Hashability;

namespace detail {

template<HashableMode mode, class Child>
inline consteval
void verify_hashable_child() noexcept {

	static_assert(!refl_has_attribute<Child, HashableCategory>,
		"HashableCategory is not an attribute");
	static_assert(!refl_has_attribute<Child, HashableMode>,
		"HashableMode is not meant for fields and properties");

	using Type = ReflFieldOrPropertyType<Child>;
	if constexpr (mode == HashableMode::OptOut) {
		constexpr auto attr = refl_attribute_or<Child, Hashable, Hashable{true}>;
		static_assert(!attr || get_hashability<Type>(),
			"Unhashable field or property marked as hashable");
	}
	else if constexpr (mode == HashableMode::OptIn) {
		constexpr auto attr = refl_attribute_or<Child, Hashable, Hashable{false}>;
		static_assert(!attr || get_hashability<Type>(),
			"Unhashable field or property marked as hashable");
	}
	else if constexpr (mode == HashableMode::AsBytes) {
		static_assert(refl_attribute_or<Child, Hashable, Hashable{true}>,
			"Can't opt out from hashing a member of a byte-hashable class");
		if constexpr (is_description_property<Child>) {
			static_assert(!refl_attribute_or<Child, Hashable, Hashable{false}>,
				"Byte-hashable class can't have hashable properties");
		}
	}
	else if constexpr (mode == HashableMode::None) {
		return;
	}
	else
		static_assert(false);
}

} // namespace detail

template<class T>
using IsHashable = BoolC<bool{get_hashability<T>()}>;

template<class T>
using IsByteHashable = BoolC<get_hashability<T>().mode() == HashableMode::AsBytes>;

template<class T>
inline consteval
auto get_hashability() noexcept -> Hashability {
	using PT = std::remove_cvref_t<T>;
	using HVB = HasherVisitorBase;
	using enum HashableMode;
	using enum HashableCategory;
	static constexpr auto has_unique_repr = std::has_unique_object_representations_v<PT>;

	if constexpr (std::is_fundamental_v<PT>) {
		if constexpr (std::is_arithmetic_v<PT>) {
			return {Primitive, std::is_integral_v<PT> && has_unique_repr ? AsBytes : Default};
		}
		else if constexpr (std::is_same_v<PT, std::nullptr_t>) {
			return {Primitive, AsBytes};
		}
		else {
			return {Primitive, None};
		}
	}
	else if constexpr (is_hvb_wrapper<PT>) {
		if constexpr (is_hvb_wrapper_digest<PT>) {
			return {Wrapper, AsBytes};
		}
		else if constexpr (is_hvb_wrapper_bytes<PT>) {
			return {Wrapper, Default};
		}
		else if constexpr (is_hvb_wrapper_fixed_bytes<PT>) {
			return {Wrapper, Default};
		}
		else if constexpr (is_hvb_wrapper_string<PT>) {
			return {Wrapper, Default};
		}
		else if constexpr (is_hvb_wrapper_fixed_string<PT>) {
			return {Wrapper, Default};
		}
		else if constexpr (is_hvb_wrapper_ptr<PT>) {
			return {Wrapper, AsBytes};
		}
		else if constexpr (is_hvb_wrapper_tuple<PT>) {
			static constexpr auto good = []<class... Ts>(MpType<HVB::Tuple<Ts...>>) {
				return (true && ... && bool{get_hashability<Ts>()});
			}(mp_type<PT>);
			return good
				? Hashability{Wrapper, Default}
				: Hashability{Wrapper, None};
		}
		else if constexpr (is_hvb_wrapper_commutative_tuple<PT>) {
			static constexpr auto good = []<class... Ts>(MpType<HVB::CommutativeTuple<Ts...>>) {
				return (true && ... && bool{get_hashability<Ts>()});
			}(mp_type<PT>);
			return good
				? Hashability{Wrapper, Default}
				: Hashability{Wrapper, None};
		}
		else if constexpr (is_hvb_wrapper_range<PT> || is_hvb_wrapper_commutative_range<PT>) {
			return bool{get_hashability<std::iter_value_t<typename PT::IteratorType>>()}
				? Hashability{Wrapper, Default}
				: Hashability{Wrapper, None};
		}
		else if constexpr (is_hvb_wrapper_optional<PT>) {
			return bool{get_hashability<typename PT::ValueType>()}
				? Hashability{Wrapper, Default}
				: Hashability{Wrapper, None};
		}
		else {
			static_assert(false);
		}
	}
	else if constexpr(c_has_custom_hash<PT>) {
		return {Custom, Default};
	}
	else if constexpr (c_has_describe<PT>) {
		static_assert(!refl_has_attribute<PT, HashableCategory>,
			"HashableCategory is not an attribute");
		if constexpr (refl_has_attribute<PT, HashableMode>) {
			static_assert(!refl_has_attribute<PT, Hashable>,
				"Conflicting HashableMode and Hashable attributes");
			static constexpr auto mode = refl_attribute<PT, HashableMode>;
			if constexpr (mode == OptOut) {
				static_assert(mp_all_of<ReflBases<PT>, IsHashable>,
					"Hashable class must have hashable bases");
			}
			else if constexpr (mode == AsBytes) {
				static_assert(mp_all_of<ReflBases<PT>, IsByteHashable>,
					"Byte-hashable class must have byte-hashable bases");
				static_assert(has_unique_repr, "Byte-hashable class must have unique object"
					"representations");
				static_assert(mp_all_of<ReflDecomposition<PT>, IsByteHashable>,
					"Byte-hashable class must have byte-hashable fields");
				[]<class... Bases, class... FieldTypes>(MpList<Bases...>, MpList<FieldTypes...>) {
					static constexpr auto base_sum = (0zu + ... + sizeof(Bases));
					static constexpr auto field_sum = (0zu + ... + sizeof(FieldTypes));
					static_assert(base_sum + field_sum == sizeof(PT),
						"Extra class padding or a missing base/field description");
				}(ReflBases<PT>{}, ReflDecomposition<PT>{});
			}
			[]<class... FPs>(MpList<FPs...>) {
				(..., detail::verify_hashable_child<mode, FPs>());
			}(ReflFieldsAndProperties<PT>{});
			return {Described, mode};
		}
		else if constexpr (refl_has_attribute<PT, Hashable>) {
			static constexpr auto mode = refl_attribute<PT, Hashable>.value ? OptOut : None;
			if constexpr (mode == OptOut) {
				static_assert(mp_all_of<ReflBases<PT>, IsHashable>,
					"Hashable class must have hashable bases");
			}
			[]<class... FPs>(MpList<FPs...>) {
				(..., detail::verify_hashable_child<mode, FPs>());
			}(ReflFieldsAndProperties<PT>{});
			return {Described, mode};
		}
		else {
			return {Described, None};
		}
	}
	else if constexpr (std::is_enum_v<PT>) {
		return {Enum, get_hashability<std::underlying_type_t<PT>>().mode()};
	}
	else if constexpr (c_optional_like<PT>) {
		return {Optional, get_hashability<typename PT::value_type>() ? Default : None};
	}
	else if constexpr (c_string_like<PT> || c_string_view_like<PT>) {
		return {String, Default};
	}
	else if constexpr (std::is_bounded_array_v<PT>) {
		return {Array, get_hashability<std::remove_all_extents_t<PT>>().mode()};
	}
	else if constexpr (c_std_array_like<PT>) {
		return {Array, get_hashability<typename PT::value_type>().mode()};
	}
	else if constexpr (std::ranges::input_range<PT>) {
		return {
			Range,
			get_hashability<std::ranges::range_value_t<PT>>() ? Default : None
		};
	}
	else if constexpr (c_record_like<PT>) {
		using Decomposed = ReflDecomposition<PT>;
		if (mp_all_of<Decomposed, IsByteHashable> && has_unique_repr)
			return {Record, AsBytes};
		else if (mp_all_of<Decomposed, IsHashable>)
			return {Record, Default};
		else
			return {Record, None};
	}
	else {
		return {Unhashable, None};
	}
}

template<class T>
concept c_hashable = bool{get_hashability<std::remove_cvref_t<T>>()};

} // namespace fr
#endif // include guard
