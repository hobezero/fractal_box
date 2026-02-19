#ifndef FRACTAL_BOX_CORE_BIT_MANIP_HPP
#define FRACTAL_BOX_CORE_BIT_MANIP_HPP

#include <bit>
#include <climits>
#include <concepts>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

namespace fr {

template<class T>
inline constexpr auto bit_width = static_cast<int>(sizeof(T) * CHAR_BIT);

template<std::integral T>
inline constexpr
auto make_bitmask(int start, int width) noexcept -> T {
	FR_ASSERT_AUDIT(start >= 0 && width >= 0 && start + width <= bit_width<T>);

	using U = std::make_unsigned_t<T>;
	if (width == 0 || start == bit_width<T>)
		return T{0};
	// NOTE: Potentionally narrowing `static_cast` is necessary to workaround integral promotion
	// rules
	constexpr auto all = static_cast<U>(~U{0});
	// Clear the high bits, then clear the low bits
	const auto num_high_zeroes = bit_width<U> - width;
	const auto num_low_zeroes = bit_width<U> - (width + start);
	const auto umask = static_cast<U>(all >> num_high_zeroes << num_low_zeroes);
	if constexpr (std::is_signed_v<T>)
		return std::bit_cast<T>(umask);
	else
		return umask;
}

template<std::integral T, int Start, int Width>
requires (Start >= 0 && Width >= 0 && Start + Width <= bit_width<T>)
inline constexpr auto bitmask = make_bitmask<T>(Start, Width);

/// @brief Bitmask with `Width` lower bits set to `1`
template<std::integral T, int Width>
requires (Width >= 0 && Width <= bit_width<T>)
inline constexpr auto bitmask_low = make_bitmask<T>(bit_width<T> - Width, Width);

/// @brief Bitmask with `Width` high bits set to `1`
template<std::integral T, int Width>
requires (Width >= 0 && Width <= bit_width<T>)
inline constexpr auto bitmask_high = make_bitmask<T>(0, Width);

/// @brief The smallest `std::uint_fastX_t` type which has the size of at least `BitWidth` bits
template<int BitWidth>
using SmallestUintFastType
	= typename MpLazyIf<BitWidth <= 8>::template Type<uint_fast8_t,
		typename MpLazyIf<BitWidth <= 16>::template Type<uint_fast16_t,
			typename MpLazyIf<BitWidth <= 32>::template Type<uint_fast32_t,
				typename MpLazyIf<BitWidth <= 64>::template Type<uint_fast64_t, void>
			>
		>
	>;

} // namespace fr
#endif // include guard
