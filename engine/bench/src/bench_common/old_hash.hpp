#ifndef FRACTAL_BOX_CORE_HASHING_HASH_HPP
#define FRACTAL_BOX_CORE_HASHING_HASH_HPP

#include <climits>
#include <cmath>
#include <cstddef>

#include <bit>
#include <iterator>
#include <ranges>
#include <string>
#include <optional>
#include <string_view>

#include <city.h>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/hashing/hash_digest.hpp"
#include "fractal_box/core/hashing/hasher_utils.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

/// References:
/// @see https://abseil.io/docs/cpp/guides/hash
/// @see https://github.com/ThePhD/sol2/blob/ff3f254f7b47cfc8cf2976368d86f4cd726915ec/include/sol/stack_core.hpp#L864-L880
/// @see https://www.youtube.com/watch?v=aZNhSOIvv1Q
/// @see https://quuxplusone.github.io/blog/2018/03/19/customization-points-for-functions/
/// @see https://www.youtube.com/watch?v=T_bijOA1jts
/// @see https://github.com/facebookexperimental/libunifex/blob/main/include/unifex/tag_invoke.hpp
/// @see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3980.html
/// @see https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values/
/// @see https://codeberg.org/ziglang/zig/src/branch/master/lib/std/Io/Writer.zig
/// @see https://docs.rs/rapidhash/latest/rapidhash/inner/index.html
/// @see https://internals.rust-lang.org/t/low-latency-hashing/22010/8
/// @see https://github.com/facebook/folly/blob/main/folly/container/HeterogeneousAccess.h#L130

/// @todo TODO: Replace CityHash with something faster, constexpr-friendly, and inlinable

namespace fr {

enum class HashKind: uint8_t {
	/// @brief Hash that is guaranteed to produce the same result for the same input across
	/// **every** execution of a program
	Stable,
	/// @brief Hash that is guaranteed to produce the same result for the same input within
	/// a **single** execution of a program
	/// @details Unstable hashes make harder to find a collision which prevents a potential DDOS.
	/// The implementation may salt the initial hash value a constant itialized at runtime.
	Unstable,
};

inline constexpr
auto to_string_view(HashKind kind) noexcept -> std::string_view {
	switch (kind) {
		case HashKind::Stable: return "Stable";
		case HashKind::Unstable: return "Unstable";
	}
	FR_UNREACHABLE_MSG("Unexpected value");
}

template<c_hash_digest Digest, HashKind Kind, c_byte_like Char>
inline
auto hash_bytes(const Char* data, size_t size) noexcept -> Digest {
	const auto* const cdata = reinterpret_cast<const char*>(data);
	if constexpr (std::is_same_v<Digest, HashDigest32>) {
		if constexpr (Kind == HashKind::Stable)
			return CityHash32(cdata, size);
		else
			return hash_mix(hash_seed_unstable<Digest>, CityHash32(cdata, size));
	}
	else if constexpr (std::is_same_v<Digest, HashDigest64>) {
		if constexpr (Kind == HashKind::Stable)
			return CityHash64(cdata, size);
		else
			return CityHash64WithSeed(cdata, size, hash_seed_unstable<Digest>);
	}
	else
		static_assert(false, "Unsupported Digest type");
}

template<c_hash_digest Digest, HashKind Kind>
struct HashOps;

template<HashKind Kind>
using HashOps32 = HashOps<HashDigest32, Kind>;

template<HashKind Kind>
using HashOps64 = HashOps<HashDigest64, Kind>;

template<HashKind Kind>
using HashOpsStd = HashOps<HashDigestStd, Kind>;

template<c_hash_digest Digest>
using StableHashOps = HashOps<Digest, HashKind::Stable>;

using StableHashOps32 = HashOps<HashDigest32, HashKind::Stable>;
using StableHashOps64 = HashOps<HashDigest64, HashKind::Stable>;
using StableHashOpsStd = HashOps<HashDigestStd, HashKind::Stable>;

template<c_hash_digest Digest>
using UnstableHashOps = HashOps<Digest, HashKind::Unstable>;

using UnstableHashOps32 = HashOps<HashDigest32, HashKind::Unstable>;
using UnstableHashOps64 = HashOps<HashDigest64, HashKind::Unstable>;
using UnstableHashOpsStd = HashOps<HashDigestStd, HashKind::Unstable>;

namespace detail {

template<c_hash_digest Digest, HashKind Kind, std::integral T>
requires (sizeof(T) <= 16uz) // `uint128_t` is the largest supported type
FR_FORCE_INLINE constexpr
auto hash_value_default(T value) noexcept -> Digest {
	if constexpr (sizeof(T) <= sizeof(Digest)) {
		const auto v = static_cast<Digest>(value);
		if constexpr (Kind == HashKind::Stable)
			return murmur3_hash_number<Digest>(v);
		else
			return murmur3_hash_number_seeded<Digest>(hash_seed_unstable<Digest>, v);
	}
	else {
		if constexpr (std::is_same_v<Digest, HashDigest32>) {
			// For now let's assume that the lower 32 bits in a 64-bit hash are good enough
			return static_cast<Digest>(hash_value_default<HashDigest64, Kind>(value));
		}
		else if constexpr (sizeof(T) == 16uz && std::is_same_v<Digest, HashDigest64>) {
			if constexpr (Kind == HashKind::Stable)
				return city_hash_128_to_64(value);
			else
				return city_hash_128_to_64_seeded(hash_seed_unstable<Digest>(), value);
		}
		else
			static_assert(false, "Not implemented");
	}
}

template<c_hash_digest Digest, HashKind Kind, std::floating_point T>
inline constexpr
auto hash_value_default(T value) noexcept -> Digest {
	// The issue with `long double` is that it might have unused bytes
	if constexpr (std::is_same_v<T, long double>) {
		// Based on the Abseil algorithm
		const auto category = std::fpclassify(value);
		auto h = hash_value_default<Digest, Kind>(category);
		switch (category) {
			case FP_NORMAL: case FP_SUBNORMAL: {
				int exp;
				const auto mantissa = static_cast<double>(std::frexp(value, &exp));
				h = hash_mix_boost(h, hash_value_default<Digest, HashKind::Stable>(mantissa));
				h = hash_mix_boost(h, hash_value_default<Digest, HashKind::Stable>(exp));
				return h;
			}
			case FP_INFINITE:
				return hash_mix_boost(h, hash_value_default<Digest, HashKind::Stable>(
					std::signbit(value)));
			case FP_NAN: case FP_ZERO: default:
				return h;
		}
	}
	else {
		const auto v = std::bit_cast<HashDigestOfSize<sizeof(T)>>(value);
		return hash_value_default<Digest, Kind>(v);
	}
}

template<c_hash_digest Digest, HashKind Kind, class T>
inline constexpr
auto hash_value_default(T* ptr) noexcept -> Digest {
	const auto v = std::bit_cast<uintptr_t>(ptr);
	const auto h = hash_value_default<Digest, Kind>(v);
	return hash_mix_boost(h, h);
}

template<c_hash_digest Digest, HashKind Kind>
inline constexpr
auto hash_value_default(std::nullptr_t) noexcept -> Digest {
	constexpr auto result = hash_value_default<Digest, Kind>(static_cast<void*>(nullptr));
	return result;
}

template<c_hash_digest Digest, HashKind Kind, c_enum E>
FR_FORCE_INLINE constexpr
auto hash_value_default(E value) noexcept -> Digest {
	return hash_value_default<Digest, Kind>(static_cast<std::underlying_type_t<E>>(value));
}

template<c_hash_digest Digest, HashKind Kind, class Char, class Alloc>
inline constexpr
auto hash_value_default(
	const std::basic_string<Char, std::char_traits<Char>, Alloc>& str
) noexcept -> Digest {
	return hash_mix_boost(
		hash_bytes<Digest, Kind>(str.data(), str.size()),
		hash_value_default<Digest, HashKind::Stable>(str.size())
	);
}

template<c_hash_digest Digest, HashKind Kind, class Char>
inline constexpr
auto hash_value_default(
	std::basic_string_view<Char, std::char_traits<Char>> str
) noexcept -> Digest {
	return hash_mix_boost(
		hash_bytes<Digest, Kind>(str.data(), str.size()),
		hash_value_default<Digest, HashKind::Stable>(str.size())
	);
}

template<c_hash_digest Digest, HashKind Kind, class T>
// requires c_old_hashable_by_default<T, Digest, Kind>
inline constexpr
auto hash_value_default(const std::optional<T>& opt) noexcept -> Digest {
	if (opt) {
		return hash_mix_boost(
			Digest{0x8181},
			hash_value_default<Digest, HashKind::Stable>(*opt)
		);
	}
	else {
		return Digest{0x8080};
	}
}

// TODO: Implement `hash_value_default` for optional-like, variant-like, tuple-like, chrono-like
// types, containers, etc. (ideally without including the respective headers)

template<class T, class Digest, HashKind Kind>
concept c_old_hashable_by_default = requires(const T& object) {
	// It's ok to perform the test here since all overloads are known at this point
	{ ::fr::detail::hash_value_default<Digest, Kind>(object) } -> std::same_as<Digest>;
};

template<class T, class Digest, class KindC>
using AdlTestFrCustomHash = decltype(
	fr_old_custom_hash<HashOps<Digest, KindC::value>>(std::declval<const T&>()));

/// @note Can't use `requires` because `fr_old_custom_hash` might be declared after this header.
/// `is_detected` idiom delays the check until template instantiation
template<class T, class Digest, HashKind Kind>
concept c_custom_hashable = is_detected<AdlTestFrCustomHash, T, Digest, ValueC<Kind>>;

} // namespace detail

/// @note The client is not allowed to override the default implementation
template<class T, class Digest = HashDigestStd, HashKind Kind = HashKind::Stable>
concept c_old_hashable = detail::c_old_hashable_by_default<T, Digest, Kind>
	!= detail::c_custom_hashable<T, Digest, Kind>;

template<c_hash_digest Digest = HashDigestStd, HashKind Kind = HashKind::Stable, class T = void>
struct Hash;

template<c_hash_digest DigestType, HashKind Kind, c_old_hashable<DigestType, Kind> T>
struct Hash<DigestType, Kind, T> {
	using Digest = DigestType;
	static constexpr auto kind = Kind;
	static constexpr auto is_stable = (Kind == HashKind::Stable);
	using Argument = T;
	using Ops = HashOps<Digest, Kind>;

	// `std::hash` compatibility
	using argument_type = T;
	using result_type = Digest;

	static FR_FORCE_INLINE constexpr
	auto operator()(const T& value) noexcept -> Digest {
		if constexpr (detail::c_old_hashable_by_default<T, Digest, Kind>) {
			return detail::hash_value_default<Digest, Kind>(value);
		}
		else if constexpr (detail::c_custom_hashable<T, Digest, Kind>) {
			return fr_old_custom_hash<HashOps<Digest, Kind>>(value);
		}
		else if constexpr (Kind == HashKind::Unstable &&
			detail::c_custom_hashable<T, Digest, HashKind::Stable>
		) {
			/// TODO: Test this branch
			return hash_mix_boost(
				hash_seed_unstable<Digest>,
				fr_old_custom_hash<HashOps<Digest, HashKind::Stable>>(value)
			);
		}
		else
			static_assert(false, "Hashing is not implemented for this type");
	}
};

/// @brief "Transparent" hasher
template<c_hash_digest DigestType, HashKind Kind>
struct Hash<DigestType, Kind, void> {
	using Digest = DigestType;
	static constexpr auto hash_kind = Kind;
	static constexpr auto is_stable = (Kind == HashKind::Stable);
	using Argument = void;
	using Ops = HashOps<Digest, Kind>;

	// `std::hash` compatibility
	using result_type = Digest;
	using is_transparent = void;

	template<c_old_hashable<Digest, Kind> T>
	static FR_FORCE_INLINE constexpr
	auto operator()(const T& value) noexcept -> Digest {
		return Hash<Digest, Kind, T>{}(value);
	}
};

template<HashKind Kind, class T = void>
using Hash32 = Hash<HashDigest32, Kind, T>;

template<HashKind Kind, class T = void>
using Hash64 = Hash<HashDigest64, Kind, T>;

template<HashKind Kind, class T = void>
using HashStd = Hash<HashDigestStd, Kind, T>;

template<c_hash_digest Digest, class T = void>
using StableHash = Hash<Digest, HashKind::Stable, T>;

template<class T = void>
using StableHash32 = Hash<HashDigest32, HashKind::Stable, T>;

template<class T = void>
using StableHash64 = Hash<HashDigest64, HashKind::Stable, T>;

template<class T = void>
using StableHashStd = Hash<HashDigestStd, HashKind::Stable, T>;

template<c_hash_digest Digest, class T = void>
using UnstableHash = Hash<Digest, HashKind::Unstable, T>;

template<class T = void>
using UnstableHash32 = Hash<HashDigest32, HashKind::Unstable, T>;

template<class T = void>
using UnstableHash64 = Hash<HashDigest64, HashKind::Unstable, T>;

template<class T = void>
using UnstableHashStd = Hash<HashDigest64, HashKind::Unstable, T>;

template<c_hash_digest Digest, HashKind Kind = HashKind::Stable>
inline constexpr
auto hash_expansion() noexcept -> Digest {
	return hash_seed<Digest, Kind == HashKind::Stable>;
}

template<c_hash_digest Digest, HashKind Kind = HashKind::Stable, class First, class... Rest>
inline constexpr
auto hash_expansion(First&& first, Rest&&... rest) noexcept -> Digest {
	// NOTE: We call the unstable hash function only for the first item. There is no need to mix in
	// the seed more than once
	Digest result = Hash<Digest, Kind, std::remove_cvref_t<First>>{}(first);
	// Expands into:
	// result = hash_mix(result, StableHash<Digest, R_0>{}(r_0));
	// result = hash_mix(result, StableHash<Digest, R_1>{}(r_1));
	// ...
	// result = hash_mix(result, StableHash<Digest, R_N>{}(r_N));
	((result = hash_mix_boost(result, StableHash<Digest, std::remove_cvref_t<Rest>>{}(rest))), ...);
	return result;
}

template<c_hash_digest Digest, HashKind Kind = HashKind::Stable, class... Args>
inline constexpr
auto hash_expansion_unordered() noexcept -> Digest {
	return hash_seed<Digest, Kind == HashKind::Stable>;
}

template<c_hash_digest Digest, HashKind Kind = HashKind::Stable, class First, class... Rest>
inline constexpr
auto hash_expansion_unordered(First&& first, Rest&&... rest) noexcept -> Digest {
	if constexpr (sizeof...(Rest) == 0) {
		// Expansion of a single value is its hash
		return Hash<Digest, Kind, First>{}(first);
	}
	else {
		Digest result = StableHash<Digest, std::remove_cvref_t<First>>{}(first);
		if constexpr (Kind == HashKind::Unstable) {
			// Can't use unstable hash on the first item because it would break order-independence
			result = hash_mix_commutative(hash_seed<Digest, Kind == HashKind::Stable>, result);
		}
		// Expands into:
		// result = hash_mix_commutative(result, StableHash<Digest, R_0>{}(r_0));
		// result = hash_mix_commutative(result, StableHash<Digest, R_1>{}(r_1));
		// ...
		// result = hash_mix_commutative(result, StableHash<Digest, R_N>{}(r_N));
		((result = hash_mix_commutative(result, StableHash<Digest,
			std::remove_cvref_t<Rest>>{}(rest))), ...);
		return result;
	}
}

template<
	c_hash_digest Digest = HashDigestStd, HashKind Kind = HashKind::Stable,
	std::input_iterator Iter, std::sentinel_for<Iter> Sentinel
>
inline constexpr
auto hash_range(Iter start, Sentinel finish) noexcept -> Digest {
	using T = std::iter_value_t<Iter>;
	if (start == finish)
		return hash_seed<Digest, Kind == HashKind::Stable>;

	// NOTE: We call the unstable hash function only for the first item. There is no need to mix in
	// the seed more than once
	Digest result = Hash<Digest, Kind, T>{}(*start);
	for (Iter it = ++start; it != finish; ++it) {
		result = hash_mix_boost(result, StableHash<Digest, T>{}(*it));
	}
	return result;
}

template<c_hash_digest Digest = HashDigestStd, HashKind Kind = HashKind::Stable>
inline constexpr
auto hash_range(std::ranges::input_range auto&& range) noexcept -> Digest {
	return hash_range<Digest, Kind>(std::ranges::begin(range), std::ranges::end(range));
}

template<
	c_hash_digest Digest = HashDigestStd, HashKind Kind = HashKind::Stable,
	std::input_iterator Iter, std::sentinel_for<Iter> Sentinel
>
inline constexpr
auto hash_range_unordered(Iter start, Sentinel finish) noexcept -> Digest {
	using T = std::iter_value_t<Iter>;

	Digest result;
	if constexpr (Kind == HashKind::Stable) {
		if (start == finish)
			return hash_seed<Digest, Kind == HashKind::Stable>;
		result = Hash<Digest, Kind, T>{}(*start);
		++start;
	}
	else {
		result = hash_seed<Digest, Kind == HashKind::Stable>;
	}

	for (auto it = start; it != finish; ++it) {
		result = hash_mix_commutative(result, StableHash<Digest, T>{}(*it));
	}
	return result;
}

template<c_hash_digest Digest = HashDigestStd, HashKind Kind = HashKind::Stable>
inline constexpr
auto hash_range_unordered(std::ranges::input_range auto&& range) noexcept -> Digest {
	return hash_range_unordered<Digest, Kind>(std::ranges::begin(range), std::ranges::end(range));
}

/// @brief A wrapper around common hashing operations that is passed into used-defined
/// `catfuz_custom_hash_*` functions
/// @details The idea is that the user code doesn't need to include our header to define
/// `catfuz_custom_hash_*`
template<c_hash_digest DigestType, HashKind Kind>
struct HashOps {
	using Digest = DigestType;
	using RebindStable = HashOps<Digest, HashKind::Stable>;
	static constexpr auto kind = Kind;
	/// @brief Reports the same thing as `hash_kind` but allows the client to avoid including this
	/// header
	static constexpr auto is_stable = (kind == HashKind::Stable);
	template<class T> using HashType = Hash<Digest, kind, T>;
	static constexpr auto seed = hash_seed<Digest, Kind == HashKind::Stable>;

	static FR_FORCE_INLINE constexpr
	auto hash_mix(Digest left, Digest right) noexcept -> Digest {
		return ::fr::hash_mix_boost<Digest>(left, right);
	}

	template<class T>
	static FR_FORCE_INLINE constexpr
	auto hash_value(T&& value) noexcept -> Digest {
		return Hash<Digest, kind, std::remove_cvref_t<T>>{}(std::forward<T>(value));
	}

	template<c_byte_like Char>
	static FR_FORCE_INLINE constexpr
	auto hash_bytes(const Char* data, size_t size) noexcept -> Digest {
		return ::fr::hash_bytes<Digest, kind>(data, size);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	auto hash_expansion(Args&&... args) noexcept -> Digest {
		return ::fr::hash_expansion<Digest, kind>(std::forward<Args>(args)...);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	auto hash_expansion_unordered(Args&&... args) noexcept -> Digest {
		return ::fr::hash_expansion_unordered<Digest, kind>(std::forward<Args>(args)...);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	auto hash_range(Args&&... args) noexcept {
		return ::fr::hash_range<Digest, kind>(std::forward<Args>(args)...);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	auto hash_range_unordered(Args&&... args) noexcept {
		return ::fr::hash_range_unordered<Digest, kind>(std::forward<Args>(args)...);
	}
};

} // namespace fr
#endif // include guard
