#ifndef FRACTAL_BOX_CORE_ARRAY_UTILS_HPP
#define FRACTAL_BOX_CORE_ARRAY_UTILS_HPP

#include <algorithm>
#include <array>
#include <iterator>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/assert.hpp"

namespace fr {

template<size_t N, class R>
inline constexpr
auto make_array_n_from(R&& src) {
	if constexpr (requires { std::ranges::size(src); }) {
		FR_ASSERT(std::ranges::size(src) <= N);
	}

	using T = std::iter_value_t<decltype(std::ranges::begin(src))>;
	auto arr = std::array<T, N>{};
	std::ranges::copy(src, arr.data());
	return arr;
}

/// @note Isn't `noexcept` because of `std::to_array`
template<class T, size_t N, class CompareFunc = std::less<T>>
inline constexpr
auto make_sorted_array(T (&in)[N], CompareFunc cmp = {}) -> std::array<std::remove_cv_t<T>, N> {
	auto out = std::to_array(in);
	std::sort(out.begin(), out.end(), cmp);
	return out;
}

/// @note Isn't `noexcept` because of `std::to_array`
template<class T, size_t N, class CompareFunc = std::less<T>>
inline constexpr
auto make_sorted_array(T (&&in)[N], CompareFunc cmp = {}) -> std::array<std::remove_cv_t<T>, N> {
	auto out = std::to_array(std::move(in));
	std::sort(out.begin(), out.end(), cmp);
	return out;
}

}
#endif // include guard
