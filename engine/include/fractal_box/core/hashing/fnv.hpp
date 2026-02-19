#ifndef FRACTAL_BOX_CORE_HASHING_FNV_HPP
#define FRACTAL_BOX_CORE_HASHING_FNV_HPP

#include "fractal_box/core/hashing/hash_digest.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {
namespace detail {

template<class T>
inline constexpr auto fnv1a_basis = detail::MpIllegal{};

template<>
inline constexpr auto fnv1a_basis<HashDigest32> = HashDigest32{UINT32_C(0x811C9DC5)};

template<>
inline constexpr auto fnv1a_basis<HashDigest64> = HashDigest64{UINT64_C(0xCBF29CE484222325)};

template<class T>
inline constexpr auto fnv1a_prime = detail::MpIllegal{};

template<>
inline constexpr auto fnv1a_prime<HashDigest32> = HashDigest32{UINT32_C(0x01000193)};

template<>
inline constexpr auto fnv1a_prime<HashDigest64> = HashDigest64{UINT64_C(0x00000100000001B3)};

} // namespace detail

/// @brief Calculate Fowler-Noll-Vo hash function version 1a
/// @warning Non-cryptographic
/// Defined for 32 bit and 64 bit unsigned integers
/// @param str Pointer to array of `Char`s. Allowed to be `nullptr` iff `size == 0`
/// @param size Number of characters in `str` not including null terminator
template<c_hash_digest Digest, class Char>
requires (sizeof(Digest) >= sizeof(Char))
inline constexpr
auto fnv1a_hash_string(const Char* str, size_t size) noexcept -> Digest {
	// TODO: define a 16 bit version? Calculate 32 bit hash then xor lower 16 bits with higher
	// 16 bits
	// if constexpr (sizeof(Int) == sizeof(std::uint16_t)) {
		// fnv1a<uint32>, memcpy, ...
	// } else ...
	auto hash = detail::fnv1a_basis<Digest>;
	for (size_t i = 0; i < size; ++i) {
		hash ^= static_cast<Digest>(str[i]);
		hash *= detail::fnv1a_prime<Digest>;
	}
	return hash;
}

/// @brief Calculate unsigned 32 bit hash using Fowler-Noll-Vo hash function version 1a.
/// @warning Non-cryptographic
template<class Char>
FR_FORCE_INLINE constexpr
auto fnv1a_hash_string32(const Char* str, size_t size) noexcept -> HashDigest32 {
	return fnv1a_hash_string<HashDigest32, Char>(str, size);
}

/// @brief Calculate unsigned 64 bit hash using Fowler-Noll-Vo hash function version 1a.
/// @warning Non-cryptographic
template<class Char>
FR_FORCE_INLINE constexpr
auto fnv1a_hash_string64(const Char* str, size_t size) noexcept -> HashDigest64 {
	return fnv1a_hash_string<HashDigest64, Char>(str, size);
}

} // namespace fr
#endif // include guard
