#ifndef FRACTAL_BOX_CORE_INT128_HPP
#define FRACTAL_BOX_CORE_INT128_HPP

/// @file Rudimentary support for some 128 bit integer operations
/// Experiment with codegen on different compilers/architectures: https://godbolt.org/z/vq9dcK9s8

#include <type_traits>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

/// @see https://github.com/martinus/map_benchmark/blob/master/src/app/mixer.h
#if FR_HAS_INT128
#	if FR_COMP_GCC_EMULATED
		FR_DIAGNOSTIC_PUSH
		FR_DIAGNOSTIC_DISABLE_PEDANTIC
namespace fr {
	using int128_t = __int128;
	// NOTE: __uint128_t is undocumented: https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html
	using uint128_t = unsigned __int128;
} // namespace
		FR_DIAGNOSTIC_POP
#	else
namespace fr {
	using int128_t = ::int128_t;
	using uint128_t = ::uint128_t;
} // namespace
#	endif
#elif FR_COMP_MSVC && SIZE_MAX == UINT64_MAX
#	include <intrin.h> // for __umulh
#endif

namespace fr {

/// @brief Multiply two 64-bit numbers in 128-bit mode in software
/// @details The result of `a * b` is split: the lower 64 bits get returned, while the higher
/// 64 bits  get set through the output parameter `hi`
/// @note No hardware acceleration is used
/// @todo
///   TODO: Consider returning a struct (called UInt128Parts or something) of two uint64_t values.
///   As a result, we would get a better x64 codegen when System V calling convention is used at
///   the cost of worse codegen on Microsoft x64 ABI
FR_FORCE_INLINE constexpr
auto u64_mul_u128_emulated(uint64_t a, uint64_t b, uint64_t& hi) noexcept -> uint64_t {
	const uint64_t a_hi = a >> 32u;
	const uint64_t b_hi = b >> 32u;
	const uint64_t a_lo = static_cast<uint32_t>(a);
	const uint64_t b_lo = static_cast<uint32_t>(b);

	const uint64_t rm_0 = a_hi * b_lo;
	const uint64_t rm_1 = b_hi * a_lo;
	const uint64_t r_lo = a_lo * b_lo;

	const uint64_t t = r_lo + (rm_0 << 32u);
	const uint64_t lo = t + (rm_1 << 32u);

	const uint64_t r_hi = a_hi * b_hi;
	const uint64_t carry = static_cast<uint64_t>(t < r_lo) + static_cast<uint64_t>(lo < t);

	hi = r_hi + (rm_0 >> 32u) + (rm_1 >> 32u) + carry;
	return lo;
}

#if FR_COMP_MSVC && SIZE_MAX == UINT64_MAX
#	ifdef FR_ARCH_ARM64
#		pragma intrinsic(__umulh)
#	else
#		pragma intrinsic(_umul128)
#	endif
#endif

/// @brief Multiply two 64-bit numbers in 128-bit mode
/// @details The result of `a * b` is split: the lower 64 bits get returned, while the higher
/// 64 bits  get set through the output parameter `hi`
/// @note The function uses hardware-specific instructions if available with the fallback to
/// `u64_mul_u128_emulated(..)` if not
/// @todo
///   TODO: Consider returning a struct (called UInt128Parts or something) of two uint64_t values.
///   As a result, we would get a better x64 codegen with System V calling convention at
///   the cost of a worse codegen on Microsoft x64 ABI
FR_FORCE_INLINE constexpr
auto u64_mul_u128(uint64_t a, uint64_t b, uint64_t& hi) noexcept -> uint64_t {
#if FR_HAS_INT128
	const auto result = static_cast<uint128_t>(a) * static_cast<uint128_t>(b);
	hi = static_cast<uint64_t>(result >> 64u);
	return static_cast<uint64_t>(result);
#else
	if consteval {
		return u64_mul_u128_emulated(a, b, hi);
	}
	else {
#	if (defined(_MSC_VER) && SIZE_MAX == UINT64_MAX)
#		ifdef _M_ARM64
		hi = __umulh(a, b);
		return a * b;
#		else
		return _umul128(a, b, &hi);
#		endif
#	else
		return u64_mul_u128_emulated(a, b, hi);
#	endif
	}
#endif
}

} // namespace
#endif // include guard
