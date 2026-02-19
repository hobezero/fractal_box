#ifndef FRACTAL_BOX_CORE_CONTAINERS_SIMPLE_ARRAY_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_SIMPLE_ARRAY_HPP

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @brief Simpler version of `std::array` with fewer features and no need for `<array>` include
/// @note No support for zero-sized arrays, comparison, swap,, `std::tuple_size`, reverse iterators,
/// throwable `at(..)`
template<class T, size_t N>
struct SimpleArray {
	using ValueType = T;

	using value_type = T;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator = value_type*;
	using const_iterator = const value_type*;

	FR_FORCE_INLINE constexpr
	auto data() const noexcept -> const T* { return _elems; }

	FR_FORCE_INLINE constexpr
	auto data() noexcept -> T* { return _elems; }

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> size_type { return N; }

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto empty() const noexcept -> bool { return N == 0zu; }

	FR_FORCE_INLINE constexpr
	auto operator[](size_type i) noexcept -> reference { return _elems[i]; }

	FR_FORCE_INLINE constexpr
	auto operator[](size_type i) const noexcept -> const_reference { return _elems[i]; }

	FR_FORCE_INLINE constexpr
	auto front() const noexcept -> const_reference { return _elems[0zu]; }

	FR_FORCE_INLINE constexpr
	auto front() noexcept -> reference { return _elems[0zu]; }

	FR_FORCE_INLINE constexpr
	auto back() const noexcept -> const_reference { return _elems[N - 1zu]; }

	FR_FORCE_INLINE constexpr
	auto back() noexcept -> reference { return _elems[N - 1zu]; }

	FR_FORCE_INLINE constexpr
	auto cbegin() const noexcept -> const_iterator { return _elems; }

	FR_FORCE_INLINE constexpr
	auto begin() noexcept -> iterator { return _elems; }

	FR_FORCE_INLINE constexpr
	auto begin() const noexcept -> const_iterator { return _elems; }

	FR_FORCE_INLINE constexpr
	auto cend() const noexcept -> const_iterator { return _elems + N; }

	FR_FORCE_INLINE constexpr
	auto end() noexcept -> iterator { return _elems + N; }

	FR_FORCE_INLINE constexpr
	auto end() const noexcept -> const_iterator { return _elems + N; }

public:
	T _elems[N];
};

} // namespace fr
#endif // include guard
