#ifndef FRACTAL_BOX_CORE_META_BASICS_HPP
#define FRACTAL_BOX_CORE_META_BASICS_HPP

#include <concepts>
#include <type_traits>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

// Compile-time type-wrapped constants
// -----------------------------------

/// @brief Similar to `std::is_same_v` but for values instead of types
template<auto A, auto B>
requires std::equality_comparable_with<decltype(A), decltype(B)>
inline constexpr bool is_same_value = (A == B);

template<auto A, auto B>
concept c_same_value = (A == B);

/// @brief A small utility to disable template argument deduction for non-type template parameters.
/// Similar to `std::type_identity_t` but for values instead of types
template<auto Value>
inline constexpr auto value_identity = Value;

/// @brief Like `std::integral_constant` except without the explicit `T` template parameter
/// @todo TODO: Consider switching to `std::constant_wrapper` design
///   (see https://isocpp.org/files/papers/P2781R7.html)
template<auto V>
struct ValueC {
	using ValueType = decltype(V);
	static constexpr auto value = V;

	explicit(false) constexpr
	operator ValueType() const noexcept { return V; }

	constexpr
	auto operator()() const noexcept -> ValueType { return V; }
};

template<auto V>
inline constexpr auto value_c = ValueC<V>{};

template<bool Value> using BoolC = ValueC<Value>;
using FalseC = ValueC<false>;
using TrueC = ValueC<true>;

template<uint8_t Value> using Uint8C = ValueC<Value>;
template<uint16_t Value> using Uint16C = ValueC<Value>;
template<uint32_t Value> using Uint32C = ValueC<Value>;
template<uint64_t Value> using Uint64C = ValueC<Value>;

template<int8_t Value> using Int8C = ValueC<Value>;
template<int16_t Value> using Int16C = ValueC<Value>;
template<int32_t Value> using Int32C = ValueC<Value>;
template<int64_t Value> using Int64C = ValueC<Value>;

template<size_t Value> using SizeC = ValueC<Value>;

template<float Value> using FloatC = ValueC<Value>;
template<double Value> using DoubleC = ValueC<Value>;

template<bool Value> inline constexpr auto bool_c = BoolC<Value>{};
inline constexpr auto true_c = TrueC{};
inline constexpr auto false_c = FalseC{};

template<uint8_t Value> inline constexpr auto uint8_c = Uint8C<Value>{};
template<uint16_t Value> inline constexpr auto uint16_c = Uint16C<Value>{};
template<uint32_t Value> inline constexpr auto uint32_c = Uint32C<Value>{};
template<uint64_t Value> inline constexpr auto uint64_c = Uint64C<Value>{};

template<int8_t Value> inline constexpr auto int8_c = Int8C<Value>{};
template<int16_t Value> inline constexpr auto int16_c = Int16C<Value>{};
template<int32_t Value> inline constexpr auto int32_c = Int32C<Value>{};
template<int64_t Value> inline constexpr auto int64_c = Int64C<Value>{};

template<size_t Value> inline constexpr auto size_c = SizeC<Value>{};

template<float Value> inline constexpr auto float_c = FloatC<Value>{};
template<double Value> inline constexpr auto double_c = DoubleC<Value>{};

using NPosC = SizeC<npos>;
inline constexpr auto npos_c = NPosC{};

// Core types
// ----------

namespace detail {

/// @brief Mostly used as a substitute for undefined primary variable templates
struct MpIllegal {
	explicit
	MpIllegal() = default;

	MpIllegal(const MpIllegal&) = delete;
	auto operator=(const MpIllegal&) -> MpIllegal& = delete;

	MpIllegal(MpIllegal&&) = delete;
	auto operator=(MpIllegal&&) -> MpIllegal& = delete;

	~MpIllegal() = default;
};

} // namespace detail

template<class T>
using MpType = std::type_identity<T>;

template<class T>
inline constexpr auto mp_type = MpType<T>{};

template<class... Ts>
struct MpList { };

template<class... Ts>
inline constexpr auto mp_list = MpList<Ts...>{};

template<auto V>
struct MpValue {
	using ValueType = decltype(V);
	static constexpr auto value = V;
};

template<auto V>
inline constexpr auto mp_value = MpValue<V>{};

template<auto... Vs>
struct MpValueList { };

template<auto... Vs>
inline constexpr auto mp_value_list = MpValueList<Vs...>{};

template<auto V>
using MpTypeOfValue = decltype(V);

template<size_t I, class T>
struct MpIndexedType {
	using Index = ValueC<I>;
	static constexpr auto index = I;
	using Type = T;
};

enum class Access: uint8_t {
	None,
	ReadOnly,
	WriteOnly,
	ReadWrite,
};

// Concepts
// --------

// c_const_value_of_type
// ^^^^^^^^^^^^^^^^^^^^^

template<class T, class U>
inline constexpr auto is_const_value_of_type = false;

template<class U, U val>
inline constexpr auto is_const_value_of_type<ValueC<val>, U> = true;

template<class T, class U>
using IsConstValueOfType = BoolC<is_const_value_of_type<T, U>>;

template<class T, class U>
concept c_const_value_of_type = is_const_value_of_type<T, U>;

// c_type_list
// ^^^^^^^^^^^

template<class T>
inline constexpr auto is_type_list = false;

template<template<class...> class TList, class... Ts>
inline constexpr auto is_type_list<TList<Ts...>> = true;

template<class T>
using IsTypeList = BoolC<is_type_list<T>>;

template<class T>
concept c_type_list = is_type_list<T>;

// c_mp_list
// ^^^^^^^^^

template<class T>
inline constexpr auto is_mp_list = false;

template<class... Ts>
inline constexpr auto is_mp_list<MpList<Ts...>> = true;

template<class T>
using IsMpList = BoolC<is_mp_list<T>>;

template<class T>
concept c_mp_list = is_mp_list<T>;

template<class T>
concept c_not_mp_list = !c_mp_list<T>;

// c_value_list
// ^^^^^^^^^^^^

template<class T>
inline constexpr auto is_value_list = false;

template<template<auto...> class VList, auto... Vs>
inline constexpr auto is_value_list<VList<Vs...>> = true;

template<class T>
using IsValueList = BoolC<is_value_list<T>>;

template<class T>
concept c_value_list = is_value_list<T>();

// is_complete
// ^^^^^^^^^^^

/// @note Can't be a concept because concepts are cached
/// @note Default argument is necessary to force instantiation at every usage
template<class T, bool Value = requires(T) { sizeof(T); }>
inline constexpr auto is_complete = Value;

// Control flow
// ------------

/// @brief Partial `MpIf`
template<bool Condition>
struct MpLazyIf {
	template<class T, class U>
	using Type = U;

	template<auto A, auto B>
	static constexpr auto Value = B;
};

/// @brief Partial `MpIf`
template<>
struct MpLazyIf<true> {
	template<class T, class U>
	using Type = T;

	template<auto A, auto B>
	static constexpr auto Value = A;
};

template<bool Condition, class T, class U>
using MpIf = typename MpLazyIf<Condition>::template Type<T, U>;

// SFINAE helpers
// --------------

/// @brief A small utility that provides a compile-time `false` constant dependent on `T` parameter.
/// @details Useful for SFINAE and `static_assert`s where a type-dependent expression is needed so
/// that the check doesn't fail unconditionally
/// @note Superseded in C++23 by P2593R1 "Allowing static_assert(false)"
template<class T>
inline constexpr auto always_false = false;

/// @brief A small utility that provides a compile-time `true` constant dependent on `T` parameter
template<class T>
inline constexpr auto always_true = true;

// Basic metafunctions
// -------------------

template<class T>
using RemoveConstRef = std::remove_const_t<std::remove_reference_t<T>>;

template<class T, bool Condition>
using AddConstIf = typename MpLazyIf<Condition>::template Type<std::add_const_t<T>, T>;

// CopyConst implementation
// ------------------------

namespace detail {

template<class From>
struct CopyConstImpl {
	template<class To>
	using Type = To;
};

template<class From>
struct CopyConstImpl<const From> {
	template<class To>
	using Type = const To;
};

} // namespace detail

template<class To, class From>
using CopyConst
	= typename detail::CopyConstImpl<std::remove_reference_t<From>>::template Type<To>;

namespace detail {

template<class From>
struct CopyCvImpl {
	template<class To>
	using Type = To;
};

template<class From>
struct CopyCvImpl<const From> {
	template<class To>
	using Type = const To;
};

template<class From>
struct CopyCvImpl<const volatile From> {
	template<class To>
	using Type = const volatile To;
};

} // namespace detail

template<class To, class From>
requires (!std::is_reference_v<To>)
using CopyCv = typename detail::CopyCvImpl<std::remove_reference_t<From>>::template Type<To>;

// CopyConstRef
// ------------

namespace detail {

template<class From>
struct CopyConstRefImpl {
	template<class To>
	using Type = To;
};

// cv-qualifiers
// ^^^^^^^^^^^^^

template<class From>
struct CopyConstRefImpl<const From> {
	template<class To>
	using Type = const To;
};

template<class From>
struct CopyConstRefImpl<const volatile From> {
	template<class To>
	using Type = const To;
};

// lvalue-reference qualifiers
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

template<class From>
struct CopyConstRefImpl<From&> {
	template<class To>
	using Type = To&;
};

template<class From>
struct CopyConstRefImpl<const From&> {
	template<class To>
	using Type = const To&;
};

template<class From>
struct CopyConstRefImpl<const volatile From&> {
	template<class To>
	using Type = const To&;
};

// rvalue-reference qualifiers
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

template<class From>
struct CopyConstRefImpl<From&&> {
	template<class To>
	using Type = To&&;
};

template<class From>
struct CopyConstRefImpl<const From&&> {
	template<class To>
	using Type = const To&&;
};

template<class From>
struct CopyConstRefImpl<const volatile From&&> {
	template<class To>
	using Type = const To&&;
};

} // namespace detail

template<class To, class From>
using CopyConstRef = typename detail::CopyConstRefImpl<From>::template Type<To>;

// CopyCvRef
// ---------

namespace detail {

template<class From>
struct CopyCvRefImpl {
	template<class To>
	using Type = To;
};

// cv-qualifiers
// ^^^^^^^^^^^^^

template<class From>
struct CopyCvRefImpl<const From> {
	template<class To>
	using Type = const To;
};

template<class From>
struct CopyCvRefImpl<const volatile From> {
	template<class To>
	using Type = const volatile To;
};

// lvalue-reference qualifiers
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

template<class From>
struct CopyCvRefImpl<From&> {
	template<class To>
	using Type = To&;
};

template<class From>
struct CopyCvRefImpl<const From&> {
	template<class To>
	using Type = const To&;
};

template<class From>
struct CopyCvRefImpl<const volatile From&> {
	template<class To>
	using Type = const volatile To&;
};

// rvalue-reference qualifiers
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

template<class From>
struct CopyCvRefImpl<From&&> {
	template<class To>
	using Type = To&&;
};

template<class From>
struct CopyCvRefImpl<const From&&> {
	template<class To>
	using Type = const To&&;
};

template<class From>
struct CopyCvRefImpl<const volatile From&&> {
	template<class To>
	using Type = const volatile To&&;
};

} // namespace detail

template<class To, class From>
using CopyCvRef = typename detail::CopyCvRefImpl<From>::template Type<To>;

// Forwarding helpers
// ------------------

/// @brief Proper return type of a normally read-only getter which, however, allows to move members
/// out when called on a non-const rvalue reference
template<class Member, class Class>
using GetterRet = MpLazyIf<
	std::is_lvalue_reference_v<Class> || std::is_const_v<std::remove_reference_t<Class>>
>::template Type<std::add_const_t<Member>&, CopyConst<Member, Class>&&>;

/// @brief Reference type which has similar properties to `From`. Computed the same way as the
/// return type of `std::forward_like<From>(std::declval<To>())`
template<class To, class From>
using FwdLike = CopyConstRef<To, From>&&;

// declval
// -------

/// @bfief Equivalent to `std::declval()`
template<class T>
inline constexpr
auto declval() noexcept -> std::add_rvalue_reference_t<T> {
	static_assert(false, "declval not allowed in an evaluated context");
}

// Detection of class template instantiations
// ------------------------------------------

template<class T, template<class...> class U>
inline constexpr auto is_specialization = false;

template<template<class...> class U, class... Args>
inline constexpr auto is_specialization<U<Args...>, U> = true;

template<class T, template<class...> class U>
using IsSpecialization = BoolC<is_specialization<T, U>>;

template<class T, template<class...> class U>
concept c_specialization = is_specialization<T, U>;

template<class T, template<auto...> class U>
inline constexpr auto is_specialization_nttp = false;

template<template<auto...> class U, auto... Args>
inline constexpr auto is_specialization_nttp<U<Args...>, U> = true;

template<class T, template<auto...> class U>
using IsSpecializationNttp = BoolC<is_specialization_nttp<T, U>>;

template<class T, template<auto...> class U>
concept c_specialization_nttp = is_specialization_nttp<T, U>;

// IsDetected
// ----------

namespace detail {

template<class Enabler, template<class...> class Trait, class... Args>
struct IsDetectedImpl {
	using Type = FalseC;
	using Applied = void;
};

template<template<class...> class Trait, class... Args>
struct IsDetectedImpl<std::void_t<Trait<Args...>>, Trait, Args...> {
	using Type = TrueC;
	using Applied = Trait<Args...>;
};

} // namespace detail

template<template<class...> class Op, class... Args>
using IsDetected = typename detail::IsDetectedImpl<void, Op, Args...>::Type;

template<template<class...> class Op, class... Args>
inline constexpr auto is_detected = IsDetected<Op, Args...>{}();

// Additioonal helpers
// -------------------

namespace detail {

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_UNDEFINED_INTERNAL

/// @brief Any detection helper
struct ADH {
	template<class T>
	explicit(false)
	operator T() const noexcept;
};

/// @brief Any detection helper (except base of)
template<class T>
struct ADHEB {
	template<class U>
	requires (!std::is_base_of_v<U, T>)
	explicit(false)
	operator U() const noexcept;
};

FR_DIAGNOSTIC_POP

} // namespace detail

} // namespace fr
#endif // include guard
