#ifndef FRACTAL_BOX_CORE_CONCEPTS_HPP
#define FRACTAL_BOX_CORE_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

namespace fr {

// Wrappers around standard type traits
// ------------------------------------

template<class T>
concept c_const = std::is_const_v<T>;

template<class T>
concept c_volatile = std::is_volatile_v<T>;

template<class T>
concept c_cv_qualified = c_const<T> || c_volatile<T>;

template<class T>
concept c_lvalue_ref = std::is_lvalue_reference_v<T>;

template<class T>
concept c_rvalue_ref = std::is_rvalue_reference_v<T>;

/// @note Subsumption rules don't kick in with either `c_lvalue_ref` or `c_rvalue_ref`
template<class T>
concept c_ref = std::is_reference_v<T>;

template<class T>
concept c_enum = std::is_enum_v<T>;

template<class T>
concept c_class = std::is_class_v<T>;

template<class T>
concept c_union = std::is_union_v<T>;

template<class T>
concept c_function = std::is_function_v<T>;

template<class T>
concept c_object = std::is_object_v<T>;

template<class T>
concept c_pointer = std::is_pointer_v<T>;

template<class T>
concept c_array = std::is_array_v<T>;

template<class T>
concept c_bounded_array = std::is_bounded_array_v<T>;

template<class T>
concept c_unbounded_array = !c_bounded_array<T>;

template<class T>
concept c_scalar = std::is_scalar_v<T>;

template<class T>
concept c_compoundd = std::is_compound_v<T>;

template<class T>
concept c_empty = std::is_empty_v<T>;

template<class T>
concept c_cv_or_ref = c_cv_qualified<T> || c_ref<T>;

template<class T>
concept c_default_constructible = std::is_default_constructible_v<T>;

template<class From, class To>
concept c_implicitly_convertible_to = std::is_convertible_v<From, To>;

template<class From, class To>
concept c_convertible_to = std::convertible_to<From, To>;

template<class From, class To>
concept c_nothrow_implicitly_convertible_to = std::is_nothrow_convertible_v<From, To>;

template<class T, class... Args>
concept c_nothrow_constructible = std::is_nothrow_constructible_v<T, Args...>;

template<class T>
concept c_nothrow_copy_constructible = std::is_nothrow_copy_constructible_v<T>;

template<class T>
concept c_nothrow_move_constructible = std::is_nothrow_move_constructible_v<T>;

template<class T>
concept c_nothrow_copy_assignable = std::is_nothrow_copy_assignable_v<T>;

template<class T>
concept c_nothrow_move_assignable = std::is_nothrow_move_assignable_v<T>;

template<class T>
concept c_nothrow_destructible = std::is_nothrow_destructible_v<T>;

template<class T>
concept c_void = std::is_void_v<T>;

template<class T>
concept c_aggregate = std::is_aggregate_v<T>;

// Extra concepts
// --------------

template<class T>
concept c_character
	= std::same_as<T, signed char>
	|| std::same_as<T, unsigned char>
	|| std::same_as<T, char>
	|| std::same_as<T, wchar_t>
	|| std::same_as<T, char8_t>
	|| std::same_as<T, char16_t>
	|| std::same_as<T, char32_t>
;

template<class T>
concept c_immutable = c_const<std::remove_reference_t<T>>;

template<class T>
concept c_mutable = !c_immutable<T>;

template<class T>
concept c_immutable_param = !c_ref<T> || (c_lvalue_ref<T> && c_const<std::remove_reference_t<T>>);

template<class T, class U>
concept c_maybe_const_of = std::same_as<std::remove_const_t<T>, U>;

template<class T>
concept c_user_object = (c_class<T> || c_union<T> || c_enum<T>) && !c_cv_qualified<T>;

/// @see See diagram at https://en.cppreference.com/w/cpp/language/type
template<class T>
concept c_pure_object = c_object<T> && !c_pointer<T> && !c_cv_qualified<T> && !c_array<T>;

template<class T>
concept c_arithmetic = std::integral<T> || std::floating_point<T>;

/// @brief Check that `From` is either explicitly OR implictly convertible to `To`
/// @note Naming convention is whack (implies `c_implicitly_convertible`) in order to be consistent
/// with `std::convertible_to`
template<class From, class To>
concept c_explicitly_convertible_to =
	(c_void<From> && c_void<To>)
	|| requires { static_cast<To>(std::declval<From>()); }
;

template<class From, class To>
concept c_nothrow_explicitly_convertible_to =
	(c_void<From> && c_void<To>)
	|| requires { { static_cast<To>(std::declval<From>()) } noexcept; };

template<class T>
concept c_nothrow_movable = c_nothrow_move_constructible<T> && c_nothrow_move_assignable<T>
	&& c_nothrow_destructible<T>;

template<class T>
concept c_nothrow_copyable = c_nothrow_copy_constructible<T> && c_nothrow_copy_assignable<T>
	&& c_nothrow_destructible<T>;

template<class T>
concept c_value_wrapper = std::copyable<T> && requires(T wrapper) {
	{ wrapper.value() } -> std::convertible_to<typename T::ValueType>;
	{ T{typename T::ValueType{}} };
};

// Where should we put these?
// --------------------------

static constexpr auto assume_nothrow_ctor = true;
static constexpr auto assume_nothrow_assign = true;

} // namespace fr
#endif // include guard
