#ifndef FRACTAL_BOX_CORE_HASHING_HASH_DIGEST_HPP
#define FRACTAL_BOX_CORE_HASHING_HASH_DIGEST_HPP

#include <type_traits>
#include <string_view>
#include <concepts>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

using HashDigest16 = uint16_t;

using HashDigest32 = typename MpLazyIf<sizeof(size_t) == sizeof(uint32_t)>
	::template Type<size_t, uint32_t>;
using HashDigest64 = typename MpLazyIf<sizeof(size_t) == sizeof(uint64_t)>
	::template Type<size_t, uint64_t>;

static_assert(sizeof(size_t) == sizeof(uint32_t) || sizeof(size_t) == sizeof(uint64_t));

using HashDigestStd = size_t;

namespace detail {

template<size_t Size>
inline constexpr auto uint_of_size = MpIllegal{};

template<>
inline constexpr auto uint_of_size<1> = mp_type<uint8_t>;

template<>
inline constexpr auto uint_of_size<2> = mp_type<uint16_t>;

template<>
inline constexpr auto uint_of_size<4> = mp_type<uint32_t>;

template<>
inline constexpr auto uint_of_size<8> = mp_type<uint64_t>;

template<size_t Size>
inline constexpr auto hash_digest_of_size = MpIllegal{};

template<>
inline constexpr auto hash_digest_of_size<2> = mp_type<HashDigest16>;

template<>
inline constexpr auto hash_digest_of_size<4> = mp_type<HashDigest32>;

template<>
inline constexpr auto hash_digest_of_size<8> = mp_type<HashDigest64>;

} // namespace detail

template<size_t Size>
using UIntOfSize = typename decltype(detail::uint_of_size<Size>)::type;

template<size_t Size>
using HashDigestOfSize = typename decltype(detail::hash_digest_of_size<Size>)::type;

template<class T>
concept c_hash_digest
	= std::same_as<T, HashDigest16>
	|| std::same_as<T, HashDigest32>
	|| std::same_as<T, HashDigest64>;


template<c_hash_digest Digest>
FR_FORCE_INLINE consteval
auto hash_digest_type_name() noexcept -> std::string_view {
	if constexpr (std::is_same_v<Digest, HashDigest16>)
		return "HashDigest16";
	if constexpr (std::is_same_v<Digest, HashDigest32>)
		return "HashDigest32";
	else if constexpr (std::is_same_v<Digest, HashDigest64>)
		return "HashDigest64";
	else
		static_assert(false);
}

namespace hash_literals {

FR_FORCE_INLINE consteval
auto operator ""_digest16(unsigned long long value) noexcept {
	FR_ASSERT_AUDIT(value < static_cast<unsigned long long>(static_cast<HashDigest16>(-1)));
	return static_cast<HashDigest16>(value);
}

FR_FORCE_INLINE consteval
auto operator ""_digest32(unsigned long long value) noexcept {
	FR_ASSERT_AUDIT(value < static_cast<unsigned long long>(static_cast<HashDigest32>(-1)));
	return static_cast<HashDigest32>(value);
}

FR_FORCE_INLINE consteval
auto operator ""_digest64(unsigned long long value) noexcept {
	FR_ASSERT_AUDIT(value < static_cast<unsigned long long>(static_cast<HashDigest64>(-1)));
	return static_cast<HashDigest64>(value);
}

} // namespace hash_literals

} // namespace fr
#endif // include guard
