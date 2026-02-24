#ifndef FRACTAL_BOX_CORE_META_HPP
#define FRACTAL_BOX_CORE_META_HPP

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

// SFINAE helpers
// --------------

// mp_valid
// ^^^^^^^^

namespace detail {

template<class Enabler, template<class...> class Trait, class... Args>
struct MpValidImpl {
	using Type = FalseC;
};

template<template<class...> class Trait, class... Args>
struct MpValidImpl<std::void_t<Trait<Args...>>, Trait, Args...> {
	using Type = TrueC;
};

} // namespace detail

/// @brief Check if `Op<Args...>` is well-formed
template<template<class...> class Op, class... Args>
using MpValid = typename detail::MpValidImpl<void, Op, Args...>::Type;

template<template<class...> class Op, class... Args>
inline constexpr auto mp_valid = MpValid<Op, Args...>{}();

template<template<class...> class Op, class... Args>
concept c_mp_valid = mp_valid<Op, Args...>;

// Parameter pack meta functions
// -----------------------------

// MpPackFirst
// ^^^^^^^^^^^

namespace detail {

template<class T, class... Ts>
struct MpPackFirstImpl {
	using Type = T;
};

} // namespace detail

template<class... Ts>
using MpPackFirst = typename detail::MpPackFirstImpl<Ts...>::Type;

// Type/value list metaprogramming
// -------------------------------

// MpMakeIndexSequence
// ^^^^^^^^^^^^^^^^^^^

namespace detail {

template<class S>
struct MpIntegerSequenceImpl;

template<class T, T...Idxs>
struct MpIntegerSequenceImpl<std::integer_sequence<T, Idxs...>> {
	using Type = MpValueList<Idxs...>;
};

} // namespace detail

template<size_t N>
using MpMakeIndexSequence = typename detail::MpIntegerSequenceImpl<std::make_index_sequence<N>>
	::Type;

// MpRename
// ^^^^^^^^

namespace detail {

template<class SrcList>
struct MpRenameImpl { };

template<template<class...> class SrcList, class... Ts>
struct MpRenameImpl<SrcList<Ts...>> {
	template<template<class...> class TargetList>
	using Type = TargetList<Ts...>;
};

} // namespace detail

/// @brief Converts `SrcList<Ts...>` to `TargetList<Ts..>`
template<class SrcList, template<class...> class TargetList>
using MpRename = typename detail::MpRenameImpl<SrcList>::template Type<TargetList>;

// TpMpList
// ^^^^^^^^

namespace detail {

/// @brief Converts `TList<Ts...>` to `MpList<Ts..>`
template<class From>
struct ToMpListImpl;

template<template<class...> class TList, class... Ts>
struct ToMpListImpl<TList<Ts...>> {
	using Type = MpList<Ts...>;
};

} // namespace detail

template<class Container>
using ToMpList = detail::ToMpListImpl<Container>::Type;

// c_mp_list_of
// ^^^^^^^^^^^^

template<class T, template<class> class F>
inline constexpr auto is_mp_list_of = false;

template<class... Ts, template<class> class F>
inline constexpr auto is_mp_list_of<MpList<Ts...>, F> = (true && ... && F<Ts>{}());

template<class T, template<class> class F>
using IsMpListOf = BoolC<is_mp_list_of<T, F>>;

template<class T, template<class> class F>
concept c_mp_list_of = is_mp_list_of<T, F>;

// mp_size
// ^^^^^^^

template<class TList>
inline constexpr auto mp_size = detail::MpIllegal{};

template<template<class...> class TList, class... Ts>
inline constexpr auto mp_size<TList<Ts...>> = sizeof...(Ts);

template<template<auto...> class VList, auto... Vs>
inline constexpr auto mp_size<VList<Vs...>> = sizeof...(Vs);

template<class TList>
using MpSize = SizeC<mp_size<TList>>;

// mp_is_emptyy
// ^^^^^^^^^^^^

template<class TList>
inline constexpr auto mp_is_empty = detail::MpIllegal{};

template<template<class...> class TList, class... Ts>
inline constexpr auto mp_is_empty<TList<Ts...>> = (sizeof...(Ts) == 0zu);

template<template<auto...> class VList, auto... Vs>
inline constexpr auto mp_is_empty<VList<Vs...>> = (sizeof...(Vs) == 0zu);

template<class List>
using MpIsEmpty = BoolC<mp_is_empty<List>>;

// MpAt
// ^^^^

namespace detail {

template<size_t Step>
struct MpAtImplStep;

template<>
struct MpAtImplStep<0zu> {
	template<size_t, class T0, class... Ts>
	using Type = T0;
};

template<>
struct MpAtImplStep<1zu> {
	template<size_t, class T0, class T1, class... Ts>
	using Type = T1;
};

template<>
struct MpAtImplStep<2zu> {
	template<size_t, class T0, class T1, class T2, class... Ts>
	using Type = T2;
};

template<>
struct MpAtImplStep<3zu> {
	template<size_t, class T0, class T1, class T2, class T3, class... Ts>
	using Type = T3;
};

template<>
struct MpAtImplStep<4zu> {
	template<size_t I, class T0, class T1, class T2, class T3, class T4, class... Ts>
	using Type = T4;
};

template<>
struct MpAtImplStep<5zu> {
	template<size_t I, class T0, class T1, class T2, class T3, class T4, class T5, class... Ts>
	using Type = T5;
};

template<>
struct MpAtImplStep<6zu> {
	template<size_t I, class T0, class T1, class T2, class T3, class T4, class T5, class T6,
		class... Ts>
	using Type = T6;
};

template<>
struct MpAtImplStep<7zu> {
	template<size_t I, class T0, class T1, class T2, class T3, class T4, class T5, class T6,
		class T7, class... Ts>
	using Type = T7;
};

template<>
struct MpAtImplStep<8zu> {
	template<size_t Idx, class T0, class T1, class T2, class T3, class T4, class T5, class T6,
		class T7, class... Ts>
	using Type = typename MpAtImplStep<(Idx - 8zu > 8zu ? 8zu : Idx - 8zu)>
		::template Type<Idx - 8zu, Ts...>;
};

template<class TList>
struct MpAtImpl;

template<template<class...> class TList, class... Ts>
struct MpAtImpl<TList<Ts...>> {
	template<size_t Idx>
	using Type = typename MpAtImplStep<(Idx > 8zu ? 8zu : Idx)>::template Type<Idx, Ts...>;
};

/// @note Can't replace with just an alias template (not sure why)
template<class...Ts>
struct MpPackAtImpl {
	template<size_t Idx>
	using Type = typename MpAtImplStep<(Idx > 8zu ? 8zu : Idx)>::template Type<Idx, Ts...>;
};

} // namespace detail

/// @brief Gets the type of the `MpList` element at the given index
/// @todo Non-recursive implementation
template<class TList, auto Idx>
requires (Idx < static_cast<decltype(Idx)>(mp_size<TList>))
using MpAt = typename detail::MpAtImpl<TList>::template Type<Idx>;

/// @brief Checks whether the element of `TList` at index `Idx` is of type `T`
template<class TList, auto Idx, class T>
inline constexpr auto mp_at_idx_is = std::is_same_v<MpAt<TList, Idx>, T>;

template<class TList, auto Idx, class T>
concept c_mp_at_idx_is = std::same_as<MpAt<TList, Idx>, T>;

// mp_pack_at
// ^^^^^^^^^^

template<size_t Idx, class... Ts>
requires (Idx < sizeof...(Ts))
using MpPackAt = typename detail::MpPackAtImpl<Ts...>::template Type<Idx>;

template<size_t Idx, class... Args>
FR_FORCE_INLINE constexpr
auto mp_pack_at(Args&&... args) noexcept -> MpPackAt<Idx, Args...>&& {
	using Ret = MpPackAt<Idx, Args...>&&;
	return [&]<size_t... Ns>(std::index_sequence<Ns...>) FR_FORCE_INLINE_L -> Ret {
		return [](
			decltype(reinterpret_cast<const void*>(Ns))..., auto* nth, auto* ...
		) FR_FORCE_INLINE_L -> Ret {
			return static_cast<Ret>(*nth);
		}(std::addressof(args)...);
	}(std::make_index_sequence<Idx>{});
}

// mp_find
// ^^^^^^^

namespace detail {

template<class TList, class T>
struct MpFindImpl;

template<template<class...> class TList, class... Ts, class T>
struct MpFindImpl<TList<Ts...>, T> {
	static constexpr
	auto get_value() noexcept -> size_t {
		auto i = 0zu;
		const bool found = ((false || ... || (std::is_same_v<Ts, T> ? true : (++i, false))));
		return found ? i : npos;
	}
};

} // namespace detail

template<class TList, class T>
inline constexpr auto mp_find = detail::MpFindImpl<TList, T>::get_value();

/// @brief Returns the index of the first occurence of `T` in `TList` if `TList` contains `T`,
/// `npos` otherwise
template<class TList, class T>
using MpFind = SizeC<mp_find<TList, T>>;

// mp_contains
// ^^^^^^^^^^^

/// @brief Checks whether `TList` contains type `T`
template<class TList, class T>
inline constexpr auto mp_contains = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, class T>
inline constexpr auto mp_contains<TList<Ts...>, T> = (false || ... || std::is_same_v<T, Ts>);

template<class TList, class T>
using MpContains = BoolC<mp_contains<TList, T>>;

/// @brief Checks whether `TList` contains type `T`
template<class TList, class T>
concept c_mp_contains = mp_contains<TList, T>;

// mp_pack_contains
// ^^^^^^^^^^^^^^^^

template<class T, class... Ts>
inline constexpr auto mp_pack_contains = (false || ... || std::is_same_v<T, Ts>);

template<class T, class... Ts>
using MpPackContains = BoolC<mp_pack_contains<T, Ts...>>;

template<class T, class... Ts>
concept c_mp_pack_contains = mp_pack_contains<T, Ts...>;

// mp_contains_once
// ^^^^^^^^^^^^^^^^

/// @brief Checks whether `TList` contains type `T` exactly once
template<class TList, class T>
inline constexpr auto mp_contains_once = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, class T>
inline constexpr auto mp_contains_once<TList<Ts...>, T>
	= (0zu + ... + (std::is_same_v<T, Ts> ? 1zu : 0zu)) == 1zu;

template<class TList, class T>
using MpContainsOnce = BoolC<mp_contains_once<TList, T>>;

/// @brief Checks whether `TList` contains type `T` exactly once
template<class TList, class T>
concept c_mp_contains_once = mp_contains_once<TList, T>;

// mp_pack_contains_once
// ^^^^^^^^^^^^^^^^^^^^^

template<class T, class... Ts>
inline constexpr auto mp_pack_contains_once
	= (0zu + ... + (std::is_same_v<T, Ts> ? 1zu : 0zu)) == 1zu;

template<class T, class... Ts>
using MpPackContainsOnce = BoolC<mp_pack_contains_once<T, Ts...>>;

template<class T, class... Ts>
concept c_mp_pack_contains_once = mp_pack_contains_once<T, Ts...>;

// mp_count
// ^^^^^^^^

template<class TList, class T>
inline constexpr auto mp_count = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, class T>
inline constexpr auto mp_count<TList<Ts...>, T> = (0zu + ... + (std::is_same_v<Ts, T> ? 1zu : 0zu));

template<class TList, class T>
using MpCount = SizeC<mp_count<TList, T>>;

// mp_pack_count
// ^^^^^^^^^^^^^

template<class T, class... Ts>
inline constexpr auto mp_pack_count = (0zu + ... + (std::is_same_v<Ts, T> ? 1zu : 0zu));

template<class T, class... Ts>
using MpPackCount = SizeC<mp_count<T, Ts...>>;

// mp_is_unique
// ^^^^^^^^^^^^

template<class TList>
inline constexpr auto mp_is_unique = detail::MpIllegal{};

template<template<class...> class TList, class... Ts>
inline constexpr auto mp_is_unique<TList<Ts...>>
	= (true && ... && mp_pack_contains_once<Ts, Ts...>);

template<class TList>
using MpIsUnique = BoolC<mp_is_unique<TList>>;

template<class TList>
concept c_mp_unique = mp_is_unique<TList>;

// mp_pack_is_unique
// ^^^^^^^^^^^^^^^^^

template<class... Ts>
inline constexpr auto mp_pack_is_unique = (true && ... && mp_pack_contains_once<Ts, Ts...>);

template<class... Ts>
using MpPackIsUnique = BoolC<mp_pack_is_unique<Ts...>>;

template<class... Ts>
concept c_mp_unique_pack = mp_pack_is_unique<Ts...>;

// mp_is_subset
// ^^^^^^^^^^^^

template<class SubList, class SuperList>
inline constexpr auto mp_is_subset = detail::MpIllegal{};

template<
	template<class...> class SubList,
	class... SubTypes,
	template<class...> class SuperList,
	class... SuperTypes
>
inline constexpr auto mp_is_subset<SubList<SubTypes...>, SuperList<SuperTypes...>>
	= (true && ... && (mp_pack_count<SubTypes, SubTypes...>
		<= mp_pack_count<SubTypes, SuperTypes...>));

template<class SubList, class SuperList>
using MpIsSubset = BoolC<mp_is_subset<SubList, SuperList>>;

template<class SubList, class SuperList>
concept c_mp_subset_of = mp_is_subset<SubList, SuperList>;

// mp_all_of
// ^^^^^^^^^

template<class TList, template<class> class F>
inline constexpr auto mp_all_of = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, template<class> class F>
inline constexpr auto mp_all_of<TList<Ts...>, F> = (true && ... && F<Ts>{});

template<class TList, template<class> class F>
using MpAllOf = BoolC<mp_all_of<TList, F>>;

template<class TList, template<class> class F>
concept c_mp_all_of = mp_all_of<TList, F>();

// mp_none_of
// ^^^^^^^^^^

template<class TList, template<class> class F>
inline constexpr auto mp_none_of = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, template<class> class F>
inline constexpr auto mp_none_of<TList<Ts...>, F> = (true && ... && !F<Ts>{});

template<class TList, template<class> class F>
using MpNoneOf = BoolC<mp_none_of<TList, F>>;

template<class TList, template<class> class F>
concept c_mp_none_of = mp_none_of<TList, F>();

// mp_any_of
// ^^^^^^^^^

template<class TList, template<class> class F>
inline constexpr auto mp_any_of = detail::MpIllegal{};

template<template<class...> class TList, class... Ts, template<class> class F>
inline constexpr auto mp_any_of<TList<Ts...>, F> = (false || ... || F<Ts>{});

template<class TList, template<class> class F>
using MpAnyOf = BoolC<mp_any_of<TList, F>>;

template<class TList, template<class> class F>
concept c_mp_any_of = mp_any_of<TList, F>();

// MpRepack
// ^^^^^^^^

namespace detail {

template<class TList>
struct MpRepackImpl;

template<template<class...> class TList, class... Ts>
struct MpRepackImpl<TList<Ts...>> {
	template<template<class...> class F>
	using Type = F<Ts...>;
};

template<class TList>
struct MpVRepackImpl;

template<template<auto...> class TList, auto... Vs>
struct MpVRepackImpl<TList<Vs...>> {
	template<template<auto...> class F>
	using Type = F<Vs...>;
};

} // namespace detail

template<class TList, template<class...> class F>
using MpRepack = typename detail::MpRepackImpl<TList>::template Type<F>;

template<class TList, template<auto...> class F>
using MpVRepack = typename detail::MpVRepackImpl<TList>::template Type<F>;

// MpConcat
// ^^^^^^^^

namespace detail {

template<class...>
struct MpConcatImpl;

template<>
struct MpConcatImpl<> {
	using Type = MpList<>;
};

template<template<class...> class TList, class... Ts>
struct MpConcatImpl<TList<Ts...>> {
	using Type = TList<Ts...>;
};

template<
	template<class...> class TList,
	template<class...> class UList,
	class... TsA,
	class... TsB,
	class... Rest
>
struct MpConcatImpl<TList<TsA...>, UList<TsB...>, Rest...> {
	using Type = typename MpConcatImpl<TList<TsA..., TsB...>, Rest...>::Type;
};

template<template<auto...> class VList, auto... Vs>
struct MpConcatImpl<VList<Vs...>> {
	using Type = VList<Vs...>;
};

template<template<auto...> class VList, auto... VsA, auto... VsB, class... Rest>
struct MpConcatImpl<VList<VsA...>, VList<VsB...>, Rest...> {
	using Type = typename MpConcatImpl<VList<VsA..., VsB...>, Rest...>::Type;
};

} // namespace detail

template<class... TLists>
using MpConcat = typename detail::MpConcatImpl<TLists...>::Type;

// MpTransform
// ^^^^^^^^^^^

namespace detail {

template<class TList>
struct MpTransformImpl;

template<template<class...> class TList, class... Ts>
struct MpTransformImpl<TList<Ts...>> {
	template<template<class> class F>
	using Type = TList<F<Ts>...>;
};

template<class TList>
struct MpTransformVtoTImpl;

template<template<auto...> class VList, auto... Vs>
struct MpTransformVtoTImpl<VList<Vs...>> {
	template<template<class...> class TList, template<auto> class F>
	using Type = TList<F<Vs>...>;
};

} // namespace detail

template<class TList, template<class...> class F>
using MpTransform = typename detail::MpTransformImpl<TList>::template Type<F>;

template<class VList, template<auto...> class F, template<class...> class TList = MpList>
using MpTransformVtoT = typename detail::MpTransformVtoTImpl<VList>::template Type<TList, F>;

// MpValuesToTypes
// ^^^^^^^^^^^^^^^

namespace detail {

template<class VList>
struct MpValuesToTypesImpl;

template<template<class...> class TList>
struct MpValuesToTypesImpl<TList<>> {
	template<template<auto> class Proj, template<class...> class OList>
	using Type = OList<>;
};

template<template<auto...> class VList, auto... Vs>
struct MpValuesToTypesImpl<VList<Vs...>> {
	template<template<auto> class Proj, template<class...> class OList>
	using Type = OList<Proj<Vs>...>;
};

} // namespace detail

template<
	class VList,
	template<auto> class Proj = MpTypeOfValue,
	template<class...> class OList = MpList
>
using MpValuesToTypes = typename detail::MpValuesToTypesImpl<VList>::template Type<Proj, OList>;

// MpFlatten
// ^^^^^^^^^

namespace detail {

template<template<class...> class TList, class T>
struct MpFlattenOneImpl {
	using Type = TList<T>;
};

template<template<class...> class TList, class... Ts>
struct MpFlattenOneImpl<TList, TList<Ts...>> {
	using Type = MpConcat<typename MpFlattenOneImpl<TList, Ts>::Type...>;
};

template<class TList>
struct MpFlattenImpl;

template<template<class...> class TList, class... Ts>
struct MpFlattenImpl<TList<Ts...>> {
	using Type = typename MpFlattenOneImpl<TList, TList<Ts...>>::Type;
};

} // namespace detail

template<c_type_list TList>
using MpFlatten = typename detail::MpFlattenImpl<TList>::Type;

template<template<class...> class TList, class... Ts>
using MpPackFlatten = typename detail::MpFlattenOneImpl<TList, TList<Ts...>>::Type;

// MpRemoveIf & MpFilter
// ^^^^^^^^^^^^^^^^^^^^^

namespace detail {

template<bool V>
struct CondListified;

template<>
struct CondListified<true> {
	template<template<class...> class TList, class T>
	using IfType = TList<T>;

	template<template<class...> class TList, class T>
	using IfNotType = TList<>;
};

template<>
struct CondListified<false> {
	template<template<class...> class TList, class T>
	using IfType = TList<>;

	template<template<class...> class TList, class T>
	using IfNotType = TList<T>;
};

template<class TList, template<class> class Pred>
struct MpRemoveIfImpl;

template<template<class...> class TList, class... Ts, template<class> class Pred>
struct MpRemoveIfImpl<TList<Ts...>, Pred> {
	using Type = MpConcat<TList<>, typename CondListified<static_cast<bool>(Pred<Ts>{})>
		::template IfNotType<TList, Ts>...>;
};

template<class TList, template<class> class Pred, template<class> class Proj>
struct MpRemoveIfProjImpl;

template<
	template<class...> class TList,
	class... Ts,
	template<class> class Pred,
	template<class> class Proj
>
struct MpRemoveIfProjImpl<TList<Ts...>, Pred, Proj> {
	using Type = MpConcat<TList<>, typename CondListified<static_cast<bool>(Pred<Proj<Ts>>{})>
		::template IfNotType<TList, Ts>...>;
};

template<class TList, template<class> class Pred>
struct MpFilterImpl;

template<template<class...> class TList, class... Ts, template<class> class Pred>
struct MpFilterImpl<TList<Ts...>, Pred> {
	using Type = MpConcat<TList<>, typename CondListified<static_cast<bool>(Pred<Ts>{})>
		::template IfType<TList, Ts>...>;
};

template<class TList, template<class> class Pred, template<class> class Proj>
struct MpFilterProjImpl;

template<
	template<class...> class TList,
	class... Ts,
	template<class> class Pred,
	template<class> class Proj
>
struct MpFilterProjImpl<TList<Ts...>, Pred, Proj> {
	using Type = MpConcat<TList<>, typename CondListified<static_cast<bool>(Pred<Proj<Ts>>{})>
		::template IfType<TList, Ts>...>;
};

} // namespace detail

template<class TList, template<class> class P>
using MpRemoveIf = typename detail::MpRemoveIfImpl<TList, P>::Type;

template<class TList, template<class> class P>
using MpFilter = typename detail::MpFilterImpl<TList, P>::Type;

template<class TList, template<class> class Pred, template<class> class Proj>
using MpRemoveIfProj = typename detail::MpRemoveIfProjImpl<TList, Pred, Proj>::Type;

template<class TList, template<class> class Pred, template<class> class Proj>
using MpFilterProj = typename detail::MpFilterProjImpl<TList, Pred, Proj>::Type;

// MpEnumerate
// ^^^^^^^^^^^

namespace detail {

template<class IList, class TList>
struct MpEnumerateImpl;

template<size_t... Is, class... Ts, template<class...> class TList>
struct MpEnumerateImpl<std::index_sequence<Is...>, TList<Ts...>> {
	using Type = TList<MpIndexedType<Is, Ts>...>;
};

} // namespace detail

template<class TList>
using MpEnumerate = typename detail::MpEnumerateImpl<
	std::make_index_sequence<mp_size<TList>>,
	TList
>::Type;

// MpFirst
// ^^^^^^^

namespace detail {

template<class TList>
struct MpFirstImpl;

template<template<class...> class TList, class First, class... Rest>
struct MpFirstImpl<TList<First, Rest...>> {
	using Type = First;
};

template<template<auto...> class TList, auto First, auto... Rest>
struct MpFirstImpl<TList<First, Rest...>> {
	using Type = ValueC<First>;
};

template<template<auto, class...> class TList, auto First, class... Rest>
struct MpFirstImpl<TList<First, Rest...>> {
	using Type = ValueC<First>;
};

} // namespace detail

template<class TList>
using MpFirst = typename detail::MpFirstImpl<TList>::Type;

// MpSecond
// ^^^^^^^^

namespace detail {

template<class TList>
struct MpSecondImpl;

template<template<class...> class TList, class First, class Second, class... Rest>
struct MpSecondImpl<TList<First, Second, Rest...>> {
	using Type = Second;
};

template<template<auto...> class TList, auto First, auto Second, auto... Rest>
struct MpSecondImpl<TList<First, Second, Rest...>> {
	using Type = ValueC<Second>;
};

template<template<auto, class...> class TList, auto First, class Second, class... Rest>
struct MpSecondImpl<TList<First, Second, Rest...>> {
	using Type = Second;
};

} // namespace detail

template<class TList>
using MpSecond = typename detail::MpSecondImpl<TList>::Type;

// Runtime functions
// -----------------

// for_each_type
// ^^^^^^^^^^^^^

template<template<class...> class TList, class... Ts, class F>
inline constexpr
void for_each_type(TList<Ts...>, F&& callback)
noexcept(noexcept((static_cast<void>(std::forward<F>(callback).template operator()<Ts>()), ...))) {
	(..., static_cast<void>(std::forward<F>(callback).template operator()<Ts>()));
}

template<class TList, class F>
inline constexpr
void for_each_type(F&& callback)
noexcept(noexcept((for_each_type(TList{}, std::forward<F>(callback))))) {
	for_each_type(TList{}, std::forward<F>(callback));
}

// unroll
// ^^^^^^

template<size_t N, class F>
FR_FORCE_INLINE constexpr
auto unroll(F&& callback)
noexcept(
	[]<size_t... Is>(std::index_sequence<Is...>) {
		return noexcept(
			(..., static_cast<void>(std::forward<F>(std::declval<F&>()).template operator()<Is>()))
		);
	}(std::make_index_sequence<N>{})
) {
	if constexpr (N != 0zu) {
		using Result = decltype(std::forward<F>(callback).template operator()<0zu>());
		if constexpr (std::is_void_v<Result>) {
			[&callback]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
				(..., static_cast<void>(std::forward<F>(callback).template operator()<Is>()));
			}(std::make_index_sequence<N>{});
		}
		else {
			[&callback]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
				(true && ... && std::forward<F>(callback).template operator()<Is>());
			}(std::make_index_sequence<N>{});
		}
	}
}

} // namespace fr
#endif // include guard
