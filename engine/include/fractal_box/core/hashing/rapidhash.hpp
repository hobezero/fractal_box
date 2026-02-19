#ifndef FRACTAL_BOX_CORE_HASHING_RAPIDHASH_HPP
#define FRACTAL_BOX_CORE_HASHING_RAPIDHASH_HPP

/*
 * rapidhash V3 - Very fast, high quality, platform-independent hashing algorithm.
 *
 * Based on 'wyhash', by Wang Yi <godspeed_china@yeah.net>
 *
 * Copyright (C) 2025 Nicolas De Carli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * You can contact the author at:
 *   - rapidhash source repository: https://github.com/Nicoshev/rapidhash
 */

#include <cstring>

#include <bit>
#include <memory>
#include <new>

#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/int128.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @note `IsCompact` parameter value:
///   - `true`: Normal behavior
///   - `false`: Improves large input speed, but increases code size and worsens small input speed
/// @note `IsProtected` parameter value:
///   - `true`: Extra protection against entropy loss
///   - `false`: Normal behavior, max speed

namespace detail {

constexpr uint64_t rapidhash_secret[8] = {
	UINT64_C(0x2D358DCCAA6C78A5),
	UINT64_C(0x8BB84B93962EACC9),
	UINT64_C(0x4B33A62ED433D4A3),
	UINT64_C(0x4D5A2DA51DE1AA47),
	UINT64_C(0xA0761D6478BD642F),
	UINT64_C(0xE7037ED1A0B428DB),
	UINT64_C(0x90ED1765281C388C),
	UINT64_C(0xAAAAAAAAAAAAAAAA)
};

/// @brief 64*64 -> 128bit multiply function.
/// @param A  Address of 64-bit number.
/// @param B  Address of 64-bit number.
/// @details Calculates 128-bit C = *A * *B.
/// When `IsProtected == false`:
///   Overwrites A contents with C's low 64 bits.
///   Overwrites B contents with C's high 64 bits.
/// When `IsProtected == false`:
///   Xors and overwrites A contents with C's low 64 bits.
///   Xors and overwrites B contents with C's high 64 bits.
template<bool IsProtected>
FR_FORCE_INLINE constexpr
void rapidhash_mum(uint64_t* a, uint64_t* b) noexcept {
	uint64_t hi;
	uint64_t lo = u64_mul_u128(*a, *b, hi);
	if constexpr (IsProtected) {
		*a ^= lo;
		*b ^= hi;
	}
	else {
		*a = lo;
		*b = hi;
	}
}

///  @brief Multiply and xor mix function
///  @param A  64-bit number.
///  @param B  64-bit number.
///  @details Calculates 128-bit C = A * B. Returns 64-bit xor between high and low 64 bits of C.
template<bool IsProtected>
FR_FORCE_INLINE constexpr
auto rapidhash_mix(uint64_t a, uint64_t b) noexcept -> uint64_t {
	rapidhash_mum<IsProtected>(&a, &b);
	return a ^ b;
}

template<size_t Size, bool IsAvalanching, bool IsProtected>
FR_FORCE_INLINE constexpr
auto rapidhash_0_16(
	const unsigned char* key,
	uint64_t seed,
	const uint64_t* secret = detail::rapidhash_secret
) noexcept -> uint64_t {
	const uint8_t* p = key;
	static const auto len = static_cast<uint64_t>(Size);
	seed ^= detail::rapidhash_mix<IsProtected>(seed ^ secret[2], secret[1]);
	uint64_t a = 0;
	uint64_t b = 0;
	if constexpr (len >= 4) {
		seed ^= len;
		if constexpr (len >= 8) {
			const uint8_t* plast = p + len - 8;
			a = read64(p);
			b = read64(plast);
		}
		else {
			const uint8_t* plast = p + len - 4;
			a = read32_to_64(p);
			b = read32_to_64(plast);
		}
	} else if constexpr (len > 0) {
		a = (static_cast<uint64_t>(p[0]) << 45) | p[len - 1];
		b = p[len>>1];
	} else {
		a = b = 0;
	}

	a ^= secret[1];
	b ^= seed;
	detail::rapidhash_mum<IsProtected>(&a, &b);

	if constexpr (IsAvalanching)
		return detail::rapidhash_mix<IsProtected>(a ^ secret[7], b ^ secret[1] ^ len);
	else
		return a ^ b;
}

/// @brief rapidhash main function, returns a 64-bit hash
/// @param key Buffer to be hashed
/// @param len @key length, in bytes
/// @param seed 64-bit seed used to alter the hash result predictably
/// @param secret Triplet of 64-bit secrets used to alter hash result predictably
template<bool IsAvalanching, bool IsProtected, bool IsCompact>
FR_FORCE_INLINE constexpr
auto rapidhash_internal(
	const unsigned char* key,
	size_t size,
	uint64_t seed,
	const uint64_t* secret
) noexcept -> uint64_t {
	const uint8_t* p = key;
	auto len = static_cast<uint64_t>(size);
	seed ^= rapidhash_mix<IsProtected>(seed ^ secret[2], secret[1]);
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t remaining = len;
	if (FR_LIKELY(len <= 16)) {
		if (len >= 4) {
			seed ^= len;
			if (len >= 8) {
				const uint8_t* plast = p + len - 8;
				a = read64(p);
				b = read64(plast);
			}
			else {
				const uint8_t* plast = p + len - 4;
				a = read32_to_64(p);
				b = read32_to_64(plast);
			}
		}
		else if (len > 0) {
			a = (static_cast<uint64_t>(p[0]) << 45) | p[len - 1];
			b = p[len >> 1];
		}
		else {
			a = b = 0;
		}
	}
	else {
		if (len > 112) {
			uint64_t see1 = seed;
			uint64_t see2 = seed;
			uint64_t see3 = seed;
			uint64_t see4 = seed;
			uint64_t see5 = seed;
			uint64_t see6 = seed;
			if constexpr (IsCompact) {
				do {
					seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[0],
						read64(p + 8) ^ seed);
					see1 = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[1],
						read64(p + 24) ^ see1);
					see2 = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[2],
						read64(p + 40) ^ see2);
					see3 = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[3],
						read64(p + 56) ^ see3);
					see4 = rapidhash_mix<IsProtected>(read64(p + 64) ^ secret[4],
						read64(p + 72) ^ see4);
					see5 = rapidhash_mix<IsProtected>(read64(p + 80) ^ secret[5],
						read64(p + 88) ^ see5);
					see6 = rapidhash_mix<IsProtected>(read64(p + 96) ^ secret[6],
						read64(p + 104) ^ see6);
					p += 112;
					remaining -= 112;
				}
				while(remaining > 112);
			}
			else {
				while (remaining > 224) {
					seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[0],
						read64(p + 8) ^ seed);
					see1 = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[1],
						read64(p + 24) ^ see1);
					see2 = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[2],
						read64(p + 40) ^ see2);
					see3 = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[3],
						read64(p + 56) ^ see3);
					see4 = rapidhash_mix<IsProtected>(read64(p + 64) ^ secret[4],
						read64(p + 72) ^ see4);
					see5 = rapidhash_mix<IsProtected>(read64(p + 80) ^ secret[5],
						read64(p + 88) ^ see5);
					see6 = rapidhash_mix<IsProtected>(read64(p + 96) ^ secret[6],
						read64(p + 104) ^ see6);

					seed = rapidhash_mix<IsProtected>(read64(p + 112) ^ secret[0],
						read64(p + 120) ^ seed);
					see1 = rapidhash_mix<IsProtected>(read64(p + 128) ^ secret[1],
						read64(p + 136) ^ see1);
					see2 = rapidhash_mix<IsProtected>(read64(p + 144) ^ secret[2],
						read64(p + 152) ^ see2);
					see3 = rapidhash_mix<IsProtected>(read64(p + 160) ^ secret[3],
						read64(p + 168) ^ see3);
					see4 = rapidhash_mix<IsProtected>(read64(p + 176) ^ secret[4],
						read64(p + 184) ^ see4);
					see5 = rapidhash_mix<IsProtected>(read64(p + 192) ^ secret[5],
						read64(p + 200) ^ see5);
					see6 = rapidhash_mix<IsProtected>(read64(p + 208) ^ secret[6],
						read64(p + 216) ^ see6);

					p += 224;
					remaining -= 224;
				}
				if (remaining > 112) {
					seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[0],
						read64(p + 8) ^ seed);
					see1 = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[1],
						read64(p + 24) ^ see1);
					see2 = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[2],
						read64(p + 40) ^ see2);
					see3 = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[3],
						read64(p + 56) ^ see3);
					see4 = rapidhash_mix<IsProtected>(read64(p + 64) ^ secret[4],
						read64(p + 72) ^ see4);
					see5 = rapidhash_mix<IsProtected>(read64(p + 80) ^ secret[5],
						read64(p + 88) ^ see5);
					see6 = rapidhash_mix<IsProtected>(read64(p + 96) ^ secret[6],
						read64(p + 104) ^ see6);
					p += 112;
					remaining -= 112;
				}
			}
			seed ^= see1;
			see2 ^= see3;
			see4 ^= see5;
			seed ^= see6;
			see2 ^= see4;
			seed ^= see2;
		}
		if (remaining > 16) {
			seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[2],
				read64(p + 8) ^ seed);
			if (remaining > 32) {
				seed = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[2],
					read64(p + 24) ^ seed);
				if (remaining > 48) {
					seed = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[1],
						read64(p + 40) ^ seed);
					if (remaining > 64) {
						seed = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[1],
							read64(p + 56) ^ seed);
						if (remaining > 80) {
							seed = rapidhash_mix<IsProtected>(read64(p + 64) ^ secret[2],
								read64(p + 72) ^ seed);
							if (remaining > 96) {
								seed = rapidhash_mix<IsProtected>(read64(p + 80) ^ secret[1],
									read64(p + 88) ^ seed);
							}
						}
					}
				}
			}
		}
		a = read64(p + remaining - 16) ^ remaining;
		b = read64(p + remaining - 8);
	}

	a ^= secret[1];
	b ^= seed;
	rapidhash_mum<IsProtected>(&a, &b);

	if constexpr (IsAvalanching)
		return rapidhash_mix<IsProtected>(a ^ secret[7], b ^ secret[1] ^ remaining);
	else
		return a ^ b;
}

/// @brief rapidhashMicro main function. Returns a 64-bit hash
/// @param key Buffer to be hashed
/// @param len @key length, in bytes
/// @param seed 64-bit seed used to alter the hash result predictably
/// @param secret Triplet of 64-bit secrets used to alter hash result predictably
template<bool IsAvalanching, bool IsProtected>
FR_FORCE_INLINE constexpr
auto rapidhash_micro_internal(
	const unsigned char* key,
	size_t size,
	uint64_t seed,
	const uint64_t* secret
) noexcept -> uint64_t {
	const uint8_t* p = key;
	const auto len = static_cast<uint64_t>(size);
	seed ^= rapidhash_mix<IsProtected>(seed ^ secret[2], secret[1]);
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t remaining = len;
	if (FR_LIKELY(len <= 16)) {
		if (len >= 4) {
			seed ^= len;
			if (len >= 8) {
				const uint8_t* plast = p + len - 8;
				a = read64(p);
				b = read64(plast);
			}
			else {
				const uint8_t* plast = p + len - 4;
				a = read32_to_64(p);
				b = read32_to_64(plast);
			}
		}
		else if (len > 0) {
			a = (static_cast<uint64_t>(p[0]) << 45) | p[len - 1];
			b = p[len >> 1];
		}
		else {
			a = b = 0;
		}
	}
	else {
		if (remaining > 80) {
			uint64_t see1 = seed;
			uint64_t see2 = seed;
			uint64_t see3 = seed;
			uint64_t see4 = seed;
			do {
				seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[0],
					read64(p + 8) ^ seed);
				see1 = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[1],
					read64(p + 24) ^ see1);
				see2 = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[2],
					read64(p + 40) ^ see2);
				see3 = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[3],
					read64(p + 56) ^ see3);
				see4 = rapidhash_mix<IsProtected>(read64(p + 64) ^ secret[4],
					read64(p + 72) ^ see4);
				p += 80;
				remaining -= 80;
			} while(remaining > 80);
			seed ^= see1;
			see2 ^= see3;
			seed ^= see4;
			seed ^= see2;
		}
		if (remaining > 16) {
			seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[2],
				read64(p + 8) ^ seed);
			if (remaining > 32) {
				seed = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[2],
					read64(p + 24) ^ seed);
				if (remaining > 48) {
					seed = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[1],
						read64(p + 40) ^ seed);
					if (remaining > 64) {
						seed = rapidhash_mix<IsProtected>(read64(p + 48) ^ secret[1],
							read64(p + 56) ^ seed);
					}
				}
			}
		}
		a = read64(p + remaining - 16) ^ remaining;
		b = read64(p + remaining - 8);
	}
	a ^= secret[1];
	b ^= seed;
	rapidhash_mum<IsProtected>(&a, &b);

	if constexpr (IsAvalanching)
		return rapidhash_mix<IsProtected>(a ^ secret[7], b ^ secret[1] ^ remaining);
	else
		return a ^ b;
}

///  @brief rapidhashNano main function. Returns a 64-bit hash
///  @param key Buffer to be hashed
///  @param len @key length, in bytes
///  @param seed 64-bit seed used to alter the hash result predictably
///  @param secret Triplet of 64-bit secrets used to alter hash result predictably
template<bool IsAvalanching, bool IsProtected>
FR_FORCE_INLINE constexpr
auto rapidhash_nano_internal(
	const unsigned char* key,
	size_t size,
	uint64_t seed,
	const uint64_t* secret
) noexcept -> uint64_t {
	const uint8_t* p = key;
	const auto len = static_cast<uint64_t>(size);
	seed ^= rapidhash_mix<IsProtected>(seed ^ secret[2], secret[1]);
	uint64_t a = 0;
	uint64_t b = 0;
	uint64_t remaining = len;
	if (FR_LIKELY(len <= 16)) {
		if (len >= 4) {
			seed ^= len;
			if (len >= 8) {
				const uint8_t* plast = p + len - 8;
				a = read64(p);
				b = read64(plast);
			}
			else {
				const uint8_t* plast = p + len - 4;
				a = read32_to_64(p);
				b = read32_to_64(plast);
			}
		} else if (len > 0) {
			a = (static_cast<uint64_t>(p[0]) << 45) | p[len - 1];
			b = p[len>>1];
		} else {
			a = b = 0;
		}
	} else {
		if (remaining > 48) {
			uint64_t see1 = seed;
			uint64_t see2 = seed;
			do {
				seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[0],
					read64(p + 8) ^ seed);
				see1 = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[1],
					read64(p + 24) ^ see1);
				see2 = rapidhash_mix<IsProtected>(read64(p + 32) ^ secret[2],
					read64(p + 40) ^ see2);
				p += 48;
				remaining -= 48;
			} while(remaining > 48);
			seed ^= see1;
			seed ^= see2;
		}
		if (remaining > 16) {
			seed = rapidhash_mix<IsProtected>(read64(p) ^ secret[2],
				read64(p + 8) ^ seed);
			if (remaining > 32) {
				seed = rapidhash_mix<IsProtected>(read64(p + 16) ^ secret[2],
					read64(p + 24) ^ seed);
			}
		}
		a = read64(p + remaining - 16) ^ remaining;
		b = read64(p + remaining - 8);
	}
	a ^= secret[1];
	b ^= seed;
	rapidhash_mum<IsProtected>(&a, &b);

	if constexpr (IsAvalanching)
		return rapidhash_mix<IsProtected>(a ^ secret[7], b ^ secret[1] ^ remaining);
	else
		return a ^ b;
}

template<class Derived>
struct RapidhashBase {
	template<c_byte_like B>
	static FR_FORCE_INLINE constexpr
	auto hash_bytes_seeded(const B* key, size_t size, uint64_t seed) noexcept -> uint64_t {
		if consteval {
			if constexpr (std::is_same_v<B, unsigned char>) {
				return Derived::hash_bytes_seeded_impl(key, size, seed);
			}
			else {
				auto* const data = new unsigned char[size];
				for (auto i = 0zu; i < size; ++i)
					data[i] = static_cast<unsigned char>(key[i]);
				const auto result = Derived::hash_bytes_seeded(data, size, seed);
				delete[] data;
				return result;
			}
		}
		else {
			return Derived::hash_bytes_seeded_impl(reinterpret_cast<const unsigned char*>(key), size,
				seed);
		}
	}

	template<size_t Size, c_byte_like B>
	static FR_FORCE_INLINE constexpr
	auto hash_bytes_short(const B* key, uint64_t seed) noexcept -> uint64_t {
		if consteval {
			if constexpr (std::is_same_v<B, unsigned char>) {
				return detail::rapidhash_0_16<Size, Derived::is_avalanching, Derived::is_protected>(
					key, seed);
			}
			else {
				unsigned char data[Size];
				for (auto i = 0zu; i < Size; ++i)
					data[i] = static_cast<unsigned char>(key[i]);
				return detail::rapidhash_0_16<Size, Derived::is_avalanching, Derived::is_protected>(
					data, seed);
			}
		}
		else {
			return detail::rapidhash_0_16<Size, Derived::is_avalanching, Derived::is_protected>(
				reinterpret_cast<const unsigned char*>(key), seed);
		}
	}

	template<c_byte_like B>
	static FR_FORCE_INLINE constexpr
	auto hash_bytes(const B* key, size_t size) noexcept -> uint64_t {
		return Derived::hash_bytes_seeded(key, size, 0);
	}

	template<size_t Size, class T>
	static FR_FORCE_INLINE constexpr
	auto hash_obj_seeded(const T& obj, uint64_t seed) noexcept -> uint64_t {
		static_assert(Size <= sizeof(T));
		if consteval {
			auto data = std::bit_cast<SimpleArray<unsigned char, sizeof(T)>>(obj);
			return Derived::hash_bytes_seeded_impl(data.data(), Size, seed);
		}
		else {
			if constexpr (Size <= Derived::max_short_size_bytes) {
				return hash_bytes_short<Size>(
					reinterpret_cast<const unsigned char*>(std::addressof(obj)), seed);
			}
			else {
				return Derived::hash_bytes_seeded_impl(
					reinterpret_cast<const unsigned char*>(std::addressof(obj)), Size, seed);
			}
		}
	}
};

} // namespace detail

template<bool IsAvalanching, bool IsProtected, bool IsCompact = true>
struct Rapidhash: public detail::RapidhashBase<Rapidhash<IsAvalanching, IsProtected, IsCompact>> {
	friend detail::RapidhashBase<Rapidhash<IsAvalanching, IsProtected, IsCompact>>;

	static constexpr auto is_avalanching = IsAvalanching;
	static constexpr auto is_protected = IsProtected;
	static constexpr auto is_compact = IsCompact;

	using Word = uint64_t;
	static constexpr auto word_size = sizeof(Word);
	static constexpr auto max_short_size_bytes = 16zu;
	static constexpr auto block_size_words = 14zu;
	static constexpr auto block_size_bytes = sizeof(Word) * block_size_words;

	struct Stream;

private:
	/// @brief rapidhash seeded hash function. Returns a 64-bit hash
	/// @details Calls rapidhash_internal using provided parameters and default secrets
	/// @param key Buffer to be hashed
	/// @param len @key length, in bytes
	/// @param seed 64-bit seed used to alter the hash result predictably
	static FR_FORCE_INLINE constexpr
	auto hash_bytes_seeded_impl(
		const unsigned char* key,
		size_t len,
		uint64_t seed
	) noexcept -> uint64_t {
		return detail::rapidhash_internal<IsAvalanching, IsProtected, IsCompact>(key, len, seed,
			detail::rapidhash_secret);
	}
};

template<bool IsAvalanching, bool IsProtected>
struct RapidhashMicro: public detail::RapidhashBase<RapidhashMicro<IsAvalanching, IsProtected>> {
	friend detail::RapidhashBase<RapidhashMicro<IsAvalanching, IsProtected>>;

	static constexpr auto is_avalanching = IsAvalanching;
	static constexpr auto is_protected = IsProtected;

	using Word = uint64_t;
	static constexpr auto word_size = sizeof(Word);
	static constexpr auto max_short_size_bytes = 16zu;
	static constexpr auto block_size_words = 10zu;
	static constexpr auto block_size_bytes = sizeof(Word) * block_size_words;

	struct Stream;

private:
	/// @brief rapidhashMicro seeded hash function. Returns a 64-bit hash
	/// @details Calls rapidhash_internal using provided parameters and default secrets. Designed
	/// for HPC and server applications, where cache misses make a noticeable performance detriment.
	/// Clang-18+ compiles it to ~140 instructions without stack usage, both on x86-64 and aarch64.
	/// Faster for sizes up to 512 bytes, just 15%-20% slower for inputs above 1kb
	/// @param key Buffer to be hashed
	/// @param len  @key length, in bytes
	/// @param seed 64-bit seed used to alter the hash result predictably
	static FR_FORCE_INLINE constexpr
	auto hash_bytes_seeded_impl(
		const unsigned char* key,
		size_t len,
		uint64_t seed
	) noexcept -> uint64_t {
		return detail::rapidhash_micro_internal<IsAvalanching, IsProtected>(key, len, seed,
			detail::rapidhash_secret);
	}
};

template<bool IsAvalanching, bool IsProtected = false>
struct RapidhashNano: public detail::RapidhashBase<RapidhashNano<IsAvalanching, IsProtected>> {
	friend detail::RapidhashBase<RapidhashNano<IsAvalanching, IsProtected>>;

	static constexpr auto is_avalanching = IsAvalanching;
	static constexpr auto is_protected = IsProtected;

	using Word = uint64_t;
	static constexpr auto word_size = sizeof(Word);
	static constexpr auto max_short_size_bytes = 16zu;
	static constexpr auto block_size_words = 6zu;
	static constexpr auto block_size_bytes = sizeof(Word) * block_size_words;

	struct Stream;

private:
	/// @brief rapidhashNano seeded hash function. Returns a 64-bit hash
	/// @details Calls rapidhash_withSeed using provided parameters and the default seed.
	/// Designed for mobile and embedded applications, where keeping a small code size is a top
	/// priority. Clang-18+ compiles it to less than 100 instructions without stack usage, both on
	/// x86-64 and aarch64. The fastest for sizes up to 48 bytes, but may be considerably slower
	/// for larger inputs.
	/// @param key Buffer to be hashed
	/// @param len @key length, in bytes
	/// @param seed 64-bit seed used to alter the hash result predictably
	static FR_FORCE_INLINE constexpr
	auto hash_bytes_seeded_impl(
		const unsigned char* key,
		size_t len,
		uint64_t seed
	) noexcept -> uint64_t {
		return detail::rapidhash_nano_internal<IsAvalanching, IsProtected>(key, len, seed,
			detail::rapidhash_secret);
	}
};

template<bool IsAvalanching, bool IsProtected, bool IsCompact>
struct Rapidhash<IsAvalanching, IsProtected, IsCompact>::Stream {
public:
	explicit FR_FORCE_INLINE constexpr
	Stream(uint64_t seed, const uint64_t* secret = detail::rapidhash_secret) noexcept:
		_seed{seed},
		_secret{secret}
	{ }

	FR_FORCE_INLINE constexpr
	auto absorb_words(const uint64_t* words, size_t count) noexcept -> uint64_t {
		const auto* p = words;
		auto remaining = count;
		if (remaining > block_size_words) {
			auto see1 = _seed;
			auto see2 = _seed;
			auto see3 = _seed;
			auto see4 = _seed;
			auto see5 = _seed;
			auto see6 = _seed;
			if constexpr (IsCompact) {
				do {
					_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[0], p[1] ^ _seed);
					see1 = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[1], p[3] ^ see1);
					see2 = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[2], p[5] ^ see2);
					see3 = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[3], p[7] ^ see3);
					see4 = detail::rapidhash_mix<IsProtected>(p[8] ^ _secret[4], p[9] ^ see4);
					see5 = detail::rapidhash_mix<IsProtected>(p[10] ^ _secret[4], p[11] ^ see5);
					see6 = detail::rapidhash_mix<IsProtected>(p[12] ^ _secret[4], p[13] ^ see6);
					p += block_size_words;
					remaining -= block_size_words;
				} while (remaining > block_size_words);
			}
			else {
				do {
					_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[0], p[1] ^ _seed);
					see1 = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[1], p[3] ^ see1);
					see2 = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[2], p[5] ^ see2);
					see3 = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[3], p[7] ^ see3);
					see4 = detail::rapidhash_mix<IsProtected>(p[8] ^ _secret[4], p[9] ^ see4);
					see5 = detail::rapidhash_mix<IsProtected>(p[10] ^ _secret[4], p[11] ^ see5);
					see6 = detail::rapidhash_mix<IsProtected>(p[12] ^ _secret[4], p[13] ^ see6);

					_seed = detail::rapidhash_mix<IsProtected>(p[14] ^ _secret[0], p[15] ^ _seed);
					see1 = detail::rapidhash_mix<IsProtected>(p[16] ^ _secret[1], p[17] ^ see1);
					see2 = detail::rapidhash_mix<IsProtected>(p[18] ^ _secret[2], p[19] ^ see2);
					see3 = detail::rapidhash_mix<IsProtected>(p[20] ^ _secret[3], p[21] ^ see3);
					see4 = detail::rapidhash_mix<IsProtected>(p[22] ^ _secret[4], p[23] ^ see4);
					see5 = detail::rapidhash_mix<IsProtected>(p[24] ^ _secret[4], p[25] ^ see5);
					see6 = detail::rapidhash_mix<IsProtected>(p[26] ^ _secret[4], p[27] ^ see6);

					p += 2zu * block_size_words;
					remaining -= 2zu * block_size_words;
				} while (remaining > 2zu * block_size_words);

				if (remaining > block_size_words) {
					_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[0], p[1] ^ _seed);
					see1 = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[1], p[3] ^ see1);
					see2 = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[2], p[5] ^ see2);
					see3 = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[3], p[7] ^ see3);
					see4 = detail::rapidhash_mix<IsProtected>(p[8] ^ _secret[4], p[9] ^ see4);
					see5 = detail::rapidhash_mix<IsProtected>(p[10] ^ _secret[4], p[11] ^ see5);
					see6 = detail::rapidhash_mix<IsProtected>(p[12] ^ _secret[4], p[13] ^ see6);

					p += block_size_words;
					remaining -= block_size_words;
				}
			}

			_seed ^= see1;
			see2 ^= see3;
			see4 ^= see5;
			_seed ^= see6;
			see2 ^= see4;
			_seed ^= see2;
		}
		if (remaining > 2zu) {
			_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[2], p[1] ^ _seed);
			if (count > 4zu) {
				_seed = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[2], p[3] ^ _seed);
				if (count > 6zu) {
					_seed = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[1], p[5] ^ _seed);
					if (count > 8zu) {
						_seed = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[1],
							p[7] ^ _seed);
						if (count > 10zu) {
							_seed = detail::rapidhash_mix<IsProtected>(p[8] ^ _secret[2],
								p[9] ^ _seed);
							if (count > 12zu) {
								_seed = detail::rapidhash_mix<IsProtected>(p[10] ^ _secret[1],
									p[11] ^ _seed);
							}
						}
					}
				}
			}
		}

		uint64_t a = p[remaining - 2] ^ remaining ^ _secret[1];
		uint64_t b = p[remaining - 1] ^ _seed;
		detail::rapidhash_mum<IsProtected>(&a, &b);

		if constexpr (IsAvalanching)
			_seed = detail::rapidhash_mix<IsProtected>(a ^ _secret[7], b ^ _secret[1] ^ remaining);
		else
			_seed = a ^ b;
		return _seed;
	}

private:
	uint64_t _seed;
	const uint64_t* _secret;
};

template<bool IsAvalanching, bool IsProtected>
struct RapidhashMicro<IsAvalanching, IsProtected>::Stream {
public:
	explicit FR_FORCE_INLINE constexpr
	Stream(uint64_t seed, const uint64_t* secret = detail::rapidhash_secret) noexcept:
		_seed{seed},
		_secret{secret}
	{ }

	FR_FORCE_INLINE constexpr
	auto absorb_words(const uint64_t* words, size_t count) noexcept -> uint64_t {
		const auto* p = words;
		auto remaining = count;
		if (remaining > block_size_words) {
			auto see1 = _seed;
			auto see2 = _seed;
			auto see3 = _seed;
			auto see4 = _seed;
			do {
				_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[0], p[1] ^ _seed);
				see1 = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[1], p[3] ^ see1);
				see2 = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[2], p[5] ^ see1);
				see3 = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[3], p[7] ^ see1);
				see4 = detail::rapidhash_mix<IsProtected>(p[8] ^ _secret[4], p[9] ^ see1);
				p += block_size_words;
				remaining -= block_size_words;
			} while (remaining > block_size_words);

			_seed ^= see1;
			see2 ^= see3;
			_seed ^= see4;
			_seed ^= see2;
		}
		if (remaining > 2zu) {
			_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[2], p[1] ^ _seed);
			if (count > 4zu) {
				_seed = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[2], p[3] ^ _seed);
				if (count > 6zu) {
					_seed = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[1], p[5] ^ _seed);
					if (count > 8zu) {
						_seed = detail::rapidhash_mix<IsProtected>(p[6] ^ _secret[1], p[7] ^ _seed);
					}
				}
			}
		}

		uint64_t a = p[remaining - 2] ^ remaining ^ _secret[1];
		uint64_t b = p[remaining - 1] ^ _seed;
		detail::rapidhash_mum<IsProtected>(&a, &b);

		if constexpr (IsAvalanching)
			_seed = detail::rapidhash_mix<IsProtected>(a ^ _secret[7], b ^ _secret[1] ^ remaining);
		else
			_seed = a ^ b;
		return _seed;
	}

private:
	uint64_t _seed;
	const uint64_t* _secret;
};

template<bool IsAvalanching, bool IsProtected>
struct RapidhashNano<IsAvalanching, IsProtected>::Stream {
public:
	explicit FR_FORCE_INLINE constexpr
	Stream(uint64_t seed, const uint64_t* secret = detail::rapidhash_secret) noexcept:
		_seed{seed},
		_secret{secret}
	{ }

	FR_FORCE_INLINE constexpr
	auto absorb_words(const uint64_t* words, size_t count) noexcept -> uint64_t {
		const auto* p = words;
		auto remaining = count;
		if (remaining > block_size_words) {
			auto see1 = _seed;
			auto see2 = _seed;
			do {
				_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[0], p[1] ^ _seed);
				see1 = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[1], p[3] ^ see1);
				see2 = detail::rapidhash_mix<IsProtected>(p[4] ^ _secret[2], p[5] ^ see1);
				p += block_size_words;
				remaining -= block_size_words;
			} while (remaining > block_size_words);

			_seed ^= see1;
			_seed ^= see2;
		}
		if (remaining > 2zu) {
			_seed = detail::rapidhash_mix<IsProtected>(p[0] ^ _secret[2], p[1] ^ _seed);
			if (count > 4zu) {
				_seed = detail::rapidhash_mix<IsProtected>(p[2] ^ _secret[2], p[3] ^ _seed);
			}
		}

		uint64_t a = p[remaining - 2] ^ remaining ^ _secret[1];
		uint64_t b = p[remaining - 1] ^ _seed;
		detail::rapidhash_mum<IsProtected>(&a, &b);

		if constexpr (IsAvalanching)
			_seed = detail::rapidhash_mix<IsProtected>(a ^ _secret[7], b ^ _secret[1] ^ remaining);
		else
			_seed = a ^ b;
		return _seed;
	}

private:
	uint64_t _seed;
	const uint64_t* _secret;
};

enum class RapidhashAlgo {
	FullCompact,
	FullNonCompact,
	Micro,
	Nano,
};

namespace detail {

template<RapidhashAlgo Algo, bool IsAvalanching, bool IsProtected>
auto get_rapidhash_algo_type() noexcept {
	if constexpr (Algo == RapidhashAlgo::FullCompact)
		return Rapidhash<IsAvalanching, IsProtected, true>{};
	else if constexpr (Algo == RapidhashAlgo::FullNonCompact)
		return Rapidhash<IsAvalanching, IsProtected, false>{};
	else if constexpr (Algo == RapidhashAlgo::Micro)
		return RapidhashMicro<IsAvalanching, IsProtected>{};
	else if constexpr (Algo == RapidhashAlgo::Nano)
		return RapidhashNano<IsAvalanching, IsProtected>{};
	else
		static_assert(false);
}

} // namespace detail

template<RapidhashAlgo Algo, bool IsAvalanching, bool IsProtected>
using RapidhashAlgoType = decltype(detail::get_rapidhash_algo_type<Algo, IsAvalanching,
	IsProtected>());

} // namespace fr
#endif // include guard
