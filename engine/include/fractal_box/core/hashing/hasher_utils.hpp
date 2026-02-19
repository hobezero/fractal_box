#ifndef FRACTAL_BOX_CORE_HASHING_HASHER_UTILS_HPP
#define FRACTAL_BOX_CORE_HASHING_HASHER_UTILS_HPP

#include <climits>
#include <cmath>

#include "fractal_box/core/hashing/hash_digest.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

namespace detail {

inline const void* const hash_seed_unstable_dummy = &hash_seed_unstable_dummy;

} // namespace detail

/// @warning Nobody tested the quality of these constants. Let's hope that the selected random
/// 16/32/64-bit primes are good enough
template<class Digest>
inline constexpr Digest hash_seed_stable;

template<>
inline constexpr HashDigest16 hash_seed_stable<HashDigest16> = UINT16_C(0x3BC3);

template<>
inline constexpr HashDigest32 hash_seed_stable<HashDigest32> = UINT32_C(0x9997E2F3);

template<>
inline constexpr HashDigest64 hash_seed_stable<HashDigest64> = UINT64_C(0xB492B66FBE98F273);

/// @brief A constant meant to prevent unexpected behavior of hash functions for zero value inputs
/// @warning Selected randomly. No idea if it's any good
template<class Digest>
inline constexpr Digest hash_seed_offset;

template<>
inline constexpr HashDigest16 hash_seed_offset<HashDigest16> = UINT16_C(0x2ABD);

template<>
inline constexpr HashDigest32 hash_seed_offset<HashDigest32> = UINT32_C(0x00786D55);

template<>
inline constexpr HashDigest64 hash_seed_offset<HashDigest64> = UINT64_C(0x007C2EF833688B41);

template<class Digest>
requires (sizeof(Digest) <= sizeof(detail::hash_seed_unstable_dummy))
inline const auto hash_seed_unstable = static_cast<Digest>(reinterpret_cast<uintptr_t>(
	detail::hash_seed_unstable_dummy));

template<c_hash_digest Digest, bool IsStable>
inline constexpr auto hash_seed = hash_seed_stable<Digest>;

template<c_hash_digest Digest>
inline const auto hash_seed<Digest, false> = hash_seed_unstable<Digest>;

/// @brief Order-dependent hash mixing. Useful to compute a single step of incremental construction
/// of an object hash value
/// @note See `boost::hash_combine()`
/// @note Implicit type conversions are banned, cast to `HashDigest32` or `HashDigest64` first
template<c_hash_digest Digest>
inline constexpr
auto hash_mix_boost(Digest left, Digest right) noexcept -> Digest {
	static constexpr auto a = static_cast<Digest>(UINT32_C(0x9E3779B9));
	return left ^ (right + a + (left << 6u) + (left >> 2u));
}

/// @brief Order-independent hash mixing
/// @note Implicit type conversions are banned, cast to `HashDigest32` or `HashDigest64` first
/// @see https://www.preprints.org/manuscript/201710.0192/v1/download
/// @see https://people.csail.mit.edu/devadas/pubs/mhashes.pdf
/// @todo Properly handle 16-bit digests
template<c_hash_digest Digest>
inline constexpr
auto hash_mix_commutative(Digest left, Digest right) noexcept -> Digest {
	static constexpr auto offset = static_cast<Digest>(3860031u);
	static constexpr auto m1 = static_cast<Digest>(2779u);
	static constexpr auto m2 = static_cast<Digest>(2u);
	return static_cast<Digest>(offset + (left + right) * m1 + (left * right * m2));
}

/// @brief Calculate a hash of a number. Implemented using finalization step from Murmur3
/// @see Source: https://github.com/martinus/robin-hood-hashing
template<c_hash_digest Digest>
inline constexpr
auto murmur3_hash_number(Digest value) noexcept -> Digest {
	// FIXME: Verfiy that the initial seeding method is any good
	Digest result = hash_seed_stable<Digest> ^ (value + hash_seed_offset<Digest>);
	if constexpr (std::is_same_v<Digest, HashDigest32>) {
		result ^= result >> 16u;
		result *= UINT32_C(0x85EBCA6B);
		result ^= result >> 13u;

		result *= UINT32_C(0xC2B2AE35);
		result ^= result >> 16u;

		return result;
	}
	else if constexpr (std::is_same_v<Digest, HashDigest64>) {
		result ^= result >> 33u;
		result *= UINT64_C(0xFF51AFD7ED558CCD);
		result ^= result >> 33u;

		result *= UINT64_C(0xC4CEB9FE1A85EC53);
		result ^= result >> 33u;

		return result;
	}
	else
		static_assert(false, "Unsupported Digest type");
}

template<c_hash_digest Digest>
inline constexpr
auto murmur3_hash_number_seeded(Digest seed, Digest value) noexcept -> Digest {
	// Just a finalization step of Murmur3
	// TODO: Better seeding mechanism
	Digest result = seed ^ (value + hash_seed_offset<Digest>);
	if constexpr (std::is_same_v<Digest, HashDigest32>) {
		result ^= result >> 16u;
		result *= UINT32_C(0x85EBCA6B);
		result ^= result >> 13u;

		result = result * UINT32_C(0xC2B2AE35);
		result ^= result >> 16u;

		return result;
	}
	else if constexpr (std::is_same_v<Digest, HashDigest64>) {
		result ^= result >> 33u;
		result *= UINT64_C(0xFF51AFD7ED558CCD);
		result ^= result >> 33u;

		result *= UINT64_C(0xC4CEB9FE1A85EC53);
		result ^= result >> 33u;

		return result;
	}
	else
		static_assert(false, "Unsupported Digest type");
}

/// @brief Reduce two 64-bit hashes into one
FR_FORCE_INLINE constexpr
auto city_hash_128_to_64(HashDigest64 hi, HashDigest64 lo) noexcept -> HashDigest64 {
	// Murmur-inspired hashing.
	static constexpr HashDigest64 mul = UINT64_C(0x9ddfea08eb382d69);
	HashDigest64 result = lo ^ hi;
	result *= mul;
	result ^= result >> 47u;
	result = (hi ^ result) * mul;
	result ^= result >> 47u;

	result *= mul;

	return result;
}

/// @brief Reduce 128-bit hash into one 64-bit hash. On its own can be used as a basic hash for
/// `uint128_t`
template<std::integral T>
requires (sizeof(T) == 16)
inline constexpr
auto city_hash_128_to_64(T value) noexcept -> HashDigest64 {
	const auto u = static_cast<std::make_unsigned_t<T>>(value);
	const auto hi = static_cast<HashDigest64>(u >> 64u);
	const auto lo = static_cast<HashDigest64>(u);
	return city_hash_128_to_64(hi, lo);
}

template<std::integral T>
requires (sizeof(T) == 16)
inline constexpr
auto city_hash_128_to_64_seeded(HashDigest64 seed, T value) noexcept -> HashDigest64 {
	// TODO: Better seeding
	const auto uvalue = static_cast<std::make_unsigned_t<T>>(value);
	const auto hi = static_cast<HashDigest64>(uvalue >> (sizeof(T) * CHAR_BIT / 2));
	const auto lo = static_cast<HashDigest64>(uvalue);
	// Murmur-inspired hashing.
	static constexpr HashDigest64 mul = UINT64_C(0x9ddfea08eb382d69);
	HashDigest64 result = seed ^ lo ^ (hi + hash_seed_offset<HashDigest64>);
	result *= mul;
	result ^= result >> 47u;
	result = (hi ^ result);
	result *= mul;

	result ^= result >> 47u;
	result *= mul;

	return result;
}

inline constexpr
auto twang_32from64(HashDigest64 key) noexcept -> HashDigest32 {
	key = (~key) + (key << 18);
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);
	return static_cast<HashDigest32>(key);
}

struct LongDoubleBuffer {
public:
	explicit FR_FORCE_INLINE constexpr
	LongDoubleBuffer(double mantissa, int16_t exponent) noexcept:
		_mantissa{mantissa},
		_exp{exponent}
	{ }

	explicit FR_FORCE_INLINE constexpr
	LongDoubleBuffer(long double value) noexcept {
		int exp;
		_mantissa = static_cast<double>(std::frexp(value, &exp));
		_exp = static_cast<int16_t>(exp);
	}

	FR_FORCE_INLINE constexpr
	auto data() const noexcept -> const unsigned char* {
		return reinterpret_cast<const unsigned char*>(this);
	}

	static FR_FORCE_INLINE consteval
	auto size() noexcept { return  sizeof(_mantissa) + sizeof(_exp); }

private:
	[[maybe_unused]]
	double _mantissa;
	[[maybe_unused]]
	int16_t _exp;
};

} // namespace fr
#endif // include guard
