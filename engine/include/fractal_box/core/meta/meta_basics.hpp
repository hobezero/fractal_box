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
struct MpValue {
	using Type = MpValue;
	/// @note This makes it compatible with `std::integral_constant`, `std::constant_wrapper`, and
	/// the like
	using type = MpValue;

	using ValueType = decltype(V);
	/// @note This makes it compatible with `std::integral_constant`, `std::constant_wrapper`, and
	/// the like
	using value_type = decltype(V);

	static constexpr auto value = V;

	explicit(false) constexpr
	operator ValueType() const noexcept { return V; }

	static constexpr
	auto operator()() noexcept -> ValueType { return V; }
};

template<auto V>
inline constexpr auto mp_value = MpValue<V>{};

template<bool Value> using BoolC = MpValue<Value>;
using FalseC = MpValue<false>;
using TrueC = MpValue<true>;

template<uint8_t Value> using Uint8C = MpValue<Value>;
template<uint16_t Value> using Uint16C = MpValue<Value>;
template<uint32_t Value> using Uint32C = MpValue<Value>;
template<uint64_t Value> using Uint64C = MpValue<Value>;

template<int8_t Value> using Int8C = MpValue<Value>;
template<int16_t Value> using Int16C = MpValue<Value>;
template<int32_t Value> using Int32C = MpValue<Value>;
template<int64_t Value> using Int64C = MpValue<Value>;

template<size_t Value> using SizeC = MpValue<Value>;

template<float Value> using FloatC = MpValue<Value>;
template<double Value> using DoubleC = MpValue<Value>;

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
struct MpType {
	using Type = T;
	using type = T;
};

template<class T>
inline constexpr auto mp_type = MpType<T>{};

template<class... Ts>
struct MpTypes { };

template<class... Ts>
inline constexpr auto mp_types = MpTypes<Ts...>{};

template<auto... Vs>
struct MpValues { };

template<auto... Vs>
inline constexpr auto mp_values = MpValues<Vs...>{};

template<size_t I, class T>
struct MpIndexedType {
	using Index = MpValue<I>;
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

// c_mp_value
// ^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_value = false;

template<auto V>
inline constexpr auto is_mp_value<MpValue<V>> = true;

template<class T>
using IsMpValue = BoolC<is_mp_value<T>>;

/// @brief Checks that type `T` is a specialization of `fr::MpValue`
template<class T>
concept c_mp_value = is_mp_value<T>;

// c_mp_value_of_type
// ^^^^^^^^^^^^^^^^^^

template<class T, class U>
inline constexpr auto is_mp_value_of_type = false;

template<class U, U Val>
inline constexpr auto is_mp_value_of_type<MpValue<Val>, U> = true;

template<class T, class U>
using IsMpValueOfType = BoolC<is_mp_value_of_type<T, U>>;

/// @brief Checks that type `T` is a specialization of `fr::MpValue` and represents a value of
/// type `U`
template<class T, class U>
concept c_mp_value_of_type = is_mp_value_of_type<T, U>;

// c_mp_value_of
// ^^^^^^^^^^^^^

template<class T, auto V>
inline constexpr auto is_mp_value_of = false;

/// @note We don't just `==` two values because to produce the same template specialization
/// two NTTPs must have effectively be "the same" (see +0.f vs -0.f as a counterexample)
template<auto TVal, auto V>
inline constexpr auto is_mp_value_of<MpValue<TVal>, V> = std::is_same_v<MpValue<TVal>, MpValue<V>>;

template<class T, auto V>
using IsMpValueOf = BoolC<is_mp_value_of<T, V>>;

/// @brief Checks that type `T` is a specialization of `fr::MpValue` and represents value `V`
template<class T, auto V>
concept c_mp_value_of = is_mp_value_of<T, V>;

// c_mp_constant
// ^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_constant = false;

template<template<auto> class T, auto V>
inline constexpr auto is_mp_constant<T<V>> = requires {
	requires std::is_same_v<typename T<V>::type, T<V>>;
	requires std::is_same_v<typename T<V>::value_type, decltype(V)>;
	requires std::is_same_v<T<V>, T<T<V>::value>>;
};

template<class T>
using IsMpConstant = BoolC<is_mp_constant<T>>;

/// @brief Checks that type `T` is a compile-time constant wrapper
template<class T>
concept c_mp_constant = is_mp_constant<T>;

// c_mp_constant_of_type
// ^^^^^^^^^^^^^^^^^^^^^

template<class T, class U>
inline constexpr auto is_mp_constant_of_type = false;

template<template<auto> class T, auto Val, class U>
requires c_mp_constant<T<Val>>
inline constexpr auto is_mp_constant_of_type<T<Val>, U> = std::is_same_v<decltype(Val), U>;

template<class T, class U>
using IsMpConstantOfType = BoolC<is_mp_constant_of_type<T, U>>;

/// @brief Checks that type `T` is a compile-time constant wrapper and represents a value of
/// type `U`
template<class T, class U>
concept c_mp_constant_of_type = is_mp_constant_of_type<T, U>;

// c_mp_constant_of
// ^^^^^^^^^^^^^^^^

template<class T, auto V>
inline constexpr auto is_mp_constant_of = false;

template<template<auto> class T, auto TVal, auto V>
requires c_mp_constant<T<TVal>>
inline constexpr auto is_mp_constant_of<T<TVal>, V> = std::is_same_v<T<TVal>, T<V>>;

template<class T, auto V>
using IsMpConstantOf = BoolC<is_mp_constant_of<T, V>>;

/// @brief Checks that type `T` is a compile-time constant wrapper and represents value `V`
template<class T, auto V>
concept c_mp_constant_of = is_mp_constant_of<T, V>;

// c_size_c
// ^^^^^^^^

template<class T>
inline constexpr auto is_size_c = false;

template<size_t V>
inline constexpr auto is_size_c<SizeC<V>> = true;

template<class T>
concept c_size_c = is_size_c<T>;

// c_mp_types
// ^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_types = false;

template<class... Ts>
inline constexpr auto is_mp_types<MpTypes<Ts...>> = true;

template<class T>
using IsMpTypes = BoolC<is_mp_types<T>>;

template<class T>
concept c_mp_types = is_mp_types<T>;

template<class T>
concept c_not_mp_types = !c_mp_types<T>;

// c_mp_values
// ^^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_values = false;

template<auto... Vs>
inline constexpr auto is_mp_values<MpValues<Vs...>> = true;

template<class T>
using IsMpValues = BoolC<is_mp_values<T>>;

template<class T>
concept c_mp_values = is_mp_values<T>;

// c_mp_type_list
// ^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_type_list = false;

template<template<class...> class TList, class... Ts>
inline constexpr auto is_mp_type_list<TList<Ts...>> = true;

template<class T>
using IsMpTypeList = BoolC<is_mp_type_list<T>>;

template<class T>
concept c_mp_type_list = is_mp_type_list<T>;

// c_mp_value_list
// ^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_mp_value_list = false;

template<template<auto...> class VList, auto... Vs>
inline constexpr auto is_mp_value_list<VList<Vs...>> = true;

template<class T>
using IsMpValueList = BoolC<is_mp_value_list<T>>;

template<class T>
concept c_mp_value_list = is_mp_value_list<T>;

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

/// @brief Useless on its own but useful as a metafunction
template<auto V>
using MpTypeOfValue = decltype(V);

template<class T>
using RemoveConstRef = std::remove_const_t<std::remove_reference_t<T>>;

template<class T, bool Condition>
using AddConstIf = typename MpLazyIf<Condition>::template Type<std::add_const_t<T>, T>;

// CopyConst
// ^^^^^^^^^

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
// ^^^^^^^^^^^^

namespace detail {

template<class From>
struct CopyConstRefImpl {
	template<class To>
	using Type = To;
};

// cv-qualifiers
// """""""""""""

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
// """""""""""""""""""""""""""

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
// """""""""""""""""""""""""""

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
// ^^^^^^^^^

namespace detail {

template<class From>
struct CopyCvRefImpl {
	template<class To>
	using Type = To;
};

// cv-qualifiers
// """""""""""""

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
// """""""""""""""""""""""""""

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
// """""""""""""""""""""""""""

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

struct ThrowingConvertible {
	template<class T>
	explicit(false) constexpr
	operator T() const { throw 1; }
};

} // namespace detail

} // namespace fr
#endif // include guard
