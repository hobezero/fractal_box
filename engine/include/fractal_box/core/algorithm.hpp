#ifndef FRACTAL_BOX_CORE_ALGORITHM_HPP
#define FRACTAL_BOX_CORE_ALGORITHM_HPP

#include <array>
#include <numeric>
#include <ranges>

#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/range_concepts.hpp"

namespace fr {

/// Complexity: O(N^2) time, O(1) space
template<
	std::forward_iterator It,
	std::sentinel_for<It> S,
	class Pred = EqualTo<>,
	class Proj = Identity,
	class Reporter = NoOp
>
requires std::indirectly_comparable<It, It, Pred, Proj, Proj>
inline constexpr
auto test_all_unique_small_reported(
	It first, S last, Pred pred = {}, Proj proj = {}, Reporter reporter = {}
) noexcept -> bool {
	bool success = true;
	for (auto a = first; a != last; ++a) {
		for (auto b = first; b != a; ++b) {
			// TODO: consider using `std::invoke(..)`
			if (pred(proj(*a), proj(*b))) {
				reporter(*a, *b);
				if constexpr (std::is_same_v<Reporter, NoOp>)
					return false;
				success = false;
			}
		}
	}
	return success;
}

/// @brief Check whether all values in a list are unique (there are no duplicates)
inline constexpr
auto test_all_unique_small(std::ranges::forward_range auto&& r) noexcept -> bool {
	const auto begin = std::ranges::begin(r);
	const auto end = std::ranges::end(r);

	for (auto a = begin; a != end; ++a) {
		for (auto b = begin; b != a; ++b) {
			if (*a == *b)
				return false;
		}
	}
	return true;
}

template<class T, size_t N>
inline constexpr
auto make_iota_array(T start = T{0}) -> std::array<T, N> {
	std::array<T, N> arr;
	std::iota(arr.begin(), arr.end(), start);
	return arr;
}

template<c_vector_like Vec, class... Us>
inline constexpr
auto prepended(Vec vec, Us&&... args) -> Vec {
	vec.emplace(vec.begin(), std::forward<Us>(args)...);
	return vec;
}

template<c_vector_like Vec, class... Us>
inline constexpr
auto appended(Vec vec, Us&&... args) -> Vec {
	vec.emplace_back(std::forward<Us>(args)...);
	return vec;
}

} // namespace fr
#endif // include guard
