#ifndef FRACTAL_BOX_CORE_COMPARE_HPP
#define FRACTAL_BOX_CORE_COMPARE_HPP

#include <compare>
#include <concepts>

#include "fractal_box/core/platform.hpp"

namespace fr {

/// @see https://www.jonathanmueller.dev/talk/cpp20-three-way-comparison/
/// @see https://en.cppreference.com/w/cpp/standard_library/synth-three-way
struct SynthThreeWay {
	template<class T, class U>
	requires std::totally_ordered_with<T, U>
	static constexpr
	auto operator()(const T& lhs, const U& rhs)
	noexcept(noexcept(lhs == rhs) && noexcept(lhs < rhs)) {
		if constexpr (std::three_way_comparable_with<T, U>)
			return lhs <=> rhs;
		else {
			if (lhs == rhs)
				return std::strong_ordering::equal;
			else if (lhs < rhs)
				return std::strong_ordering::less;
			else
				return std::strong_ordering::greater;
		}
	}
};

inline constexpr auto synth_three_way = SynthThreeWay{};

template<class T, class U = T>
requires std::totally_ordered_with<T, U>
using SynthThreeWayResult = decltype(synth_three_way(
	std::declval<const T&>(),
	std::declval<const U&>()
));

struct StrongOrderCmp {
	using is_transparent = void;

	template<class T, class U>
	static FR_FORCE_INLINE constexpr
	auto operator()(const T& lhs, const U& rhs)
	noexcept(noexcept(std::strong_order(lhs, rhs) < 0)) {
		return std::strong_order(lhs, rhs) < 0;
	}
};

} // namespace fr
#endif // include guard
