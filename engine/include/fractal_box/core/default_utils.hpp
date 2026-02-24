#ifndef FRACTAL_BOX_CORE_DEFAULT_UTILS_HPP
#define FRACTAL_BOX_CORE_DEFAULT_UTILS_HPP

#include <concepts>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include "fractal_box/core/compare.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/functional.hpp"

namespace fr {

/// @warning Beware of dangling pointers! Make sure that the array outlives the span
/// @todo
///   TODO: Rename to `as_span` to emphasize the temporary lifetime
template<class T, std::size_t N>
FR_FORCE_INLINE constexpr
auto to_span(T (&&arr)[N]) {
	return std::span<T>{arr};
}

template<class T>
requires std::is_object_v<T>
class NonDefault {
public:
	using ValueType = T;

	NonDefault() = delete;

	/// @see See `std::optional` consntructors
	template<class U = T>
	requires (std::constructible_from<T, U&&>
		&& !std::same_as<std::remove_cvref_t<U>, NonDefault>
		&& !std::same_as<std::remove_cvref_t<U>, InPlaceInit>)
	explicit(!std::is_convertible_v<U&&, T>) FR_FORCE_INLINE constexpr
	NonDefault(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>):
		_value(std::forward<U>(value))
	{ }

	template<class... Args>
	requires std::constructible_from<T, Args...>
	explicit FR_FORCE_INLINE constexpr
	NonDefault(InPlaceInit, Args&&... args)
	noexcept(std::is_nothrow_constructible_v<T, Args...>):
		_value(std::forward<Args>(args)...)
	{ }

	// Three ways of accessing the value

	template<class Self>
	FR_FORCE_INLINE constexpr
	auto value(this Self&& self) noexcept -> CopyCvRef<T, Self>&& {
		return std::forward<Self>(self)._value;
	}

	template<class Self>
	FR_FORCE_INLINE constexpr
	auto operator*(this Self&& self) noexcept -> CopyCvRef<T, Self>&& {
		return std::forward<Self>(self)._value;
	}

	FR_FORCE_INLINE constexpr
	auto operator->() noexcept -> T* { return std::addressof(_value); }

	FR_FORCE_INLINE constexpr
	auto operator->() const noexcept -> const T* { return std::addressof(_value); }

	/// @note `constexpr`-ness annd `noexcept`-ness are deduced
	/// @todo TODO: Deduce correct comparison category in case `T` doesn't provide `operator<=>`
	FR_FORCE_INLINE constexpr
	auto operator==(this PassAbi<NonDefault> lhs, PassAbi<NonDefault> rhs) noexcept -> bool
	requires std::equality_comparable<T> {
		return lhs._value == rhs._value;
	}

	FR_FORCE_INLINE constexpr
	auto operator<=>(this PassAbi<NonDefault> lhs, PassAbi<NonDefault> rhs) noexcept
	requires std::totally_ordered<T> {
		return synth_three_way(lhs._value, rhs._value);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(NonDefault& lhs, NonDefault& rhs) noexcept(std::is_nothrow_swappable_v<ValueType>)
	requires std::swappable<T> {
		using std::swap;
		swap(lhs._value, rhs._value);
	}

private:
	T _value;
};

template<std::copyable T, T DefaultValue>
class WithDefault;

namespace detail {

template<class T>
inline constexpr bool is_with_default = false;

template<class T, T DefaultValue>
inline constexpr bool is_with_default<WithDefault<T, DefaultValue>> = true;

} // namespace detail

/// @brief A thin wrapper around some value type `T` that has the `DefaultValue` value in the
/// default-constructed and moved-from states
///
/// Useful as a storage for handles in the implementation of RAII classes, as `WithDefault`
/// helps reduce the amount of boilerplate needed in move constructors and move assignment operators
/// @todo
///   TODO: Do we need deduction guides?
///   TODO: Consider other names: Disengageable, IntrusiveOptional
template<std::copyable T, T DefaultValue>
class WithDefault {
public:
	using ValueType = T;
	static constexpr T default_value = DefaultValue;

	WithDefault() = default;

	// Constructor from `T` and other types convertible to `T`
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<class U = std::remove_cv_t<T>>
	requires (std::constructible_from<T, U>
		&& !detail::is_with_default<std::remove_cvref_t<U>>
		&& !std::same_as<std::remove_cvref_t<U>, InPlaceInit>)
	explicit(!std::is_convertible_v<U, T>) FR_FORCE_INLINE constexpr
	WithDefault(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>):
		_value(std::forward<U>(value))
	{ }

	// "Copy constructors"
	// ^^^^^^^^^^^^^^^^^^^

	template<class... Args>
	requires std::constructible_from<T, Args...>
	explicit FR_FORCE_INLINE constexpr
	WithDefault(InPlaceInit, Args&&... args)
	noexcept(std::is_nothrow_constructible_v<T, Args...>):
		_value(std::forward<Args>(args)...)
	{ }

	constexpr
	WithDefault(const WithDefault& other) = default;

	template<T OtherDefaultValue>
	explicit(false) FR_FORCE_INLINE constexpr
	WithDefault(const WithDefault<T, OtherDefaultValue>& other)
	noexcept(std::is_nothrow_copy_constructible_v<T>):
		_value(other._value)
	{ }

	// "Copy assignment"
	// ^^^^^^^^^^^^^^^^^

	constexpr
	auto operator=(const WithDefault& other)
	noexcept(std::is_nothrow_copy_assignable_v<T>) -> WithDefault& = default;

	template<T OtherDefaultValue>
	requires (OtherDefaultValue != DefaultValue)
	FR_FORCE_INLINE constexpr
	auto operator=(const WithDefault<T, OtherDefaultValue>& other)
	noexcept(std::is_nothrow_copy_assignable_v<T>) -> WithDefault& {
		_value = other._value;
		return *this;
	}

	// "Move constructors"
	// ^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	WithDefault(WithDefault&& other)
	noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>):
		_value(std::move(other._value))
	{
		other._value = DefaultValue;
	}

	template<T OtherDefaultValue>
	requires (OtherDefaultValue != DefaultValue)
	explicit(false) FR_FORCE_INLINE constexpr
	WithDefault(WithDefault<T, OtherDefaultValue>&& other)
	noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>):
		_value(std::move(other._value))
	{
		other._value = OtherDefaultValue;
	}

	// "Move assignment"
	// ^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator=(WithDefault&& other) noexcept(
		std::is_nothrow_move_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>
	) -> WithDefault& {
		_value = std::move(other._value);
		other._value = DefaultValue;
		return *this;
	}

	template<T OtherDefaultValue>
	requires (OtherDefaultValue != DefaultValue)
	FR_FORCE_INLINE constexpr
	auto operator=(WithDefault<T, OtherDefaultValue>&& other) noexcept(
		std::is_nothrow_move_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>
	) -> WithDefault& {
		_value = std::move(other._value);
		other._value = OtherDefaultValue;
		return *this;
	}

	// Assignment from non-T
	// ^^^^^^^^^^^^^^^^^^^^^

	template<class U = T>
	requires (std::assignable_from<T&, U>
		&& (!c_scalar<T> || !std::same_as<std::decay_t<U>, T>)
		&& !detail::is_with_default<std::remove_cvref_t<U>>)
	FR_FORCE_INLINE constexpr
	auto operator=(U&& value) noexcept(std::is_nothrow_assignable_v<T&, U>) -> WithDefault& {
		_value = std::forward<U>(value);
		return *this;
	}

	// Just a destructor
	// ^^^^^^^^^^^^^^^^^

	~WithDefault() = default;

	// Comparison with the same WithDefault
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	auto operator==(this PassAbi<WithDefault> lhs, PassAbi<WithDefault> rhs) noexcept -> bool {
		return lhs._value == rhs._value;
	}

	auto operator<=>(this PassAbi<WithDefault> lhs, PassAbi<WithDefault> rhs) noexcept
	requires std::totally_ordered<T> {
		return synth_three_way(lhs._value, rhs._value);
	}

	// Comparison with another WithDefault
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<T OtherDefaultValue>
	requires std::equality_comparable<T>
	FR_FORCE_INLINE constexpr
	auto operator==(
		this PassAbi<WithDefault> lhs,
		const WithDefault<T, OtherDefaultValue>& rhs
	) noexcept -> bool {
		return lhs._value == rhs._value;
	}

	template<T OtherDefaultValue>
	requires std::totally_ordered<T>
	FR_FORCE_INLINE constexpr
	auto operator<=>(
		this PassAbi<WithDefault> lhs,
		const WithDefault<T, OtherDefaultValue>& rhs
	) noexcept {
		return synth_three_way(lhs._value, rhs._value);
	}

	// Comparison with `T`
	// ^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator==(this PassAbi<WithDefault> lhs, PassAbi<T> rhs) noexcept -> bool
	requires std::equality_comparable<T> {
		return lhs._value == rhs;
	}

	FR_FORCE_INLINE constexpr
	auto operator<=>(this PassAbi<WithDefault> lhs, PassAbi<T> rhs) noexcept
	requires std::totally_ordered<T> {
		return synth_three_way(lhs._value, rhs);
	}

	// Swap
	// ^^^^

	template<T OtherDefaultValue>
	requires std::swappable<T>
	FR_FORCE_INLINE constexpr
	void swap(WithDefault<T, OtherDefaultValue>& other)
	noexcept(std::is_nothrow_swappable_v<T>) {
		using std::swap;
		swap(_value, other._value);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(WithDefault& lhs, WithDefault& rhs)
	noexcept(std::is_nothrow_swappable_v<T>) requires std::swappable<T> {
		using std::swap;
		swap(lhs._value, rhs._value);
	}

	template<T RightDefaultValue>
	requires (std::swappable<T>
		// Fix overloading ambiguity with `std::swap`. The equality check is done through
		// `c_same_value` to enable short-circuiting and avoid hard error in case `T`
		// is not comparable
		&& std::equality_comparable<T>
		&& !c_same_value<DefaultValue, RightDefaultValue>)
	friend FR_FORCE_INLINE constexpr
	void swap(WithDefault& lhs, WithDefault<T, RightDefaultValue>& rhs)
	noexcept(std::is_nothrow_swappable_v<T>) {
		// Unpleasant workaround for the fact that `friend` only applies to the first argument of
		// `swap`
		lhs.swap(rhs);
	}

	// State manipulation
	// ^^^^^^^^^^^^^^^^^^

	constexpr
	auto release() noexcept(
		std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>
	) -> T {
		T old = std::move(_value);
		_value = DefaultValue;
		return old;
	}

	FR_FORCE_INLINE constexpr
	void reset() noexcept(std::is_nothrow_copy_assignable_v<T>) {
		_value = DefaultValue;
	}

	template<class... Args>
	requires (std::constructible_from<T, Args...>)
	FR_FORCE_INLINE constexpr
	auto emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) -> T& {
		_value.~T();
		// Placement new would be a more lightweight solution, but it can't be used in a constexpr
		// context
		if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
			std::construct_at(std::addressof(_value), std::forward<Args>(args)...);
		}
		else {
			try {
				std::construct_at(std::addressof(_value), std::forward<Args>(args)...);
			}
			catch(...) {
				std::construct_at(std::addressof(_value), DefaultValue);
				throw;
			}
		}

		return _value;
	}

	FR_FORCE_INLINE constexpr
	auto operator++() noexcept -> T&
	requires requires(T v) { ++v; } { return ++_value; }

	FR_FORCE_INLINE constexpr
	auto operator--() noexcept -> T&
	requires requires(T v) { --v; } { return --_value; }

	FR_FORCE_INLINE constexpr
	auto operator++(int) noexcept -> T
	requires requires(T v) { v++; } { return _value++; }

	FR_FORCE_INLINE constexpr
	auto operator--(int) noexcept -> T
	requires requires(T v) { v--; } { return _value--; }

	// Getters
	// ^^^^^^^

	FR_FORCE_INLINE constexpr
	auto is_default() const noexcept -> bool
	requires std::equality_comparable<T> {
		return _value == DefaultValue;
	}

	template<class Self>
	FR_FORCE_INLINE constexpr
	auto value(this Self&& self) noexcept -> CopyCvRef<T, Self>&& {
		return std::forward<Self>(self)._value;
	}

	template<class Self>
	FR_FORCE_INLINE constexpr
	auto operator*(this Self&& self) noexcept -> CopyCvRef<T, Self>&& {
		return std::forward<Self>(self)._value;
	}

	FR_FORCE_INLINE constexpr
	auto operator->() noexcept -> T* { return std::addressof(_value); }

	FR_FORCE_INLINE constexpr
	auto operator->() const noexcept -> const T* { return std::addressof(_value); }

private:
	template<std::copyable U, U OtherDefaultValue>
	friend class WithDefault;

	T _value = DefaultValue;
};

template<auto Value>
using WithDefaultValue = WithDefault<std::remove_cvref_t<decltype(Value)>, Value>;

} // namespace fr
#endif // include guard
