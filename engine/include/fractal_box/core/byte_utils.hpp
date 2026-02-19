#ifndef FRACTAL_BOX_CORE_BYTE_UTILS_HPP
#define FRACTAL_BOX_CORE_BYTE_UTILS_HPP

#include <cstddef>
#include <cstring>

#include <bit>
#include <concepts>
#include <iterator>
#include <memory>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/int_types.hpp"

namespace fr {

template<class T>
concept c_byte_like
	= std::same_as<T, char>
	|| std::same_as<T, unsigned char>
	|| std::same_as<T, std::byte>;

/// @see Compiler Explorer: https://godbolt.org/z/bTf4EeYP5
inline constexpr
auto read32_to_64(const unsigned char* p) noexcept -> uint64_t {
	if consteval {
		return static_cast<uint64_t>(p[0])
		 | (static_cast<uint64_t>(p[1]) << 8)
		 | (static_cast<uint64_t>(p[2]) << 16)
		 | (static_cast<uint64_t>(p[3]) << 24);
	}
	else {
		uint32_t v;
		std::memcpy(&v, p, sizeof(v));
		if constexpr (std::endian::native == std::endian::little) {
			return v;
		}
		else if constexpr (std::endian::native == std::endian::big) {
			return std::byteswap(v);
		}
		else {
			return v;
		}
	}
}

/// @see Compiler Explorer: https://godbolt.org/z/8vE7T69GW
inline constexpr
auto read64(const unsigned char* p) noexcept -> uint64_t {
	if consteval {
		return static_cast<uint64_t>(p[0])
		 | (static_cast<uint64_t>(p[1]) << 8)
		 | (static_cast<uint64_t>(p[2]) << 16)
		 | (static_cast<uint64_t>(p[3]) << 24)
		 | (static_cast<uint64_t>(p[4]) << 32)
		 | (static_cast<uint64_t>(p[5]) << 40)
		 | (static_cast<uint64_t>(p[6]) << 48)
		 | (static_cast<uint64_t>(p[7]) << 56)
		;
	}
	else {
		uint64_t v;
		std::memcpy(&v, p, sizeof(v));
		if constexpr (std::endian::native == std::endian::little) {
			return v;
		}
		else if constexpr (std::endian::native == std::endian::big) {
			return std::byteswap(v);
		}
		else {
			return v;
		}
	}
}

/// @see Compiler Explorer: https://godbolt.org/z/8vE7T69GW
template<size_t N>
requires (N <= sizeof(uint64_t))
inline constexpr
auto partial_read64(const unsigned char* p) noexcept -> uint64_t {
	if consteval {
		auto v = uint64_t{};
		for (auto i = 0zu; i < N; ++i) {
			v |= static_cast<uint64_t>(p[i]) << 8u * i;
		}
		return v;
	}
	else {
		uint64_t v;
		std::memcpy(&v, p, N);
		if constexpr (std::endian::native == std::endian::little) {
			return v;
		}
		else if constexpr (std::endian::native == std::endian::big) {
			return std::byteswap(v);
		}
		else {
			return v;
		}
	}
}

template<c_byte_like Target, class Source>
FR_FORCE_INLINE constexpr
void write_obj_as_bytes(Target* dest, const Source& src) noexcept {
	if consteval {
		if constexpr (sizeof(Source) == sizeof(Target)) {
			*dest = std::bit_cast<Target>(src);
		}
		else {
			const auto obj_bytes = std::bit_cast<SimpleArray<Target, sizeof(Source)>>(src);
			for (auto i = 0zu; i < sizeof(Source); ++i) {
				dest[i] = obj_bytes[i];
			}
		}
	}
	else {
		std::memcpy(dest, std::addressof(src), sizeof(Source));
	}
}

template<c_byte_like Target, c_character Source>
inline constexpr
void write_str_as_bytes(Target* dest, const Source* src, size_t src_size) noexcept {
	if consteval {
		for (auto i = 0zu; i < src_size; ++i) {
			if constexpr (sizeof(Source) == sizeof(Target)) {
				dest[i] = static_cast<Target>(src[i]);
			}
			else if constexpr (sizeof(Source) == sizeof(char16_t)) {
				dest[sizeof(Source) * i + 0zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]));
				dest[sizeof(Source) * i + 1zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]) << 8);
			}
			else if constexpr (sizeof(Source) == sizeof(char32_t)) {
				dest[sizeof(Source) * i + 0zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]));
				dest[sizeof(Source) * i + 1zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]) << 8);
				dest[sizeof(Source) * i + 2zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]) << 16);
				dest[sizeof(Source) * i + 3zu] = static_cast<unsigned char>(
					static_cast<uint32_t>(src[i]) << 24);
			}
			else
				static_assert(false);
		}
	}
	else {
		std::memcpy(dest, src, sizeof(Source) * src_size);
	}
}

template<c_byte_like Target, std::input_iterator SrcIter, std::sentinel_for<SrcIter> SrcSentinel>
inline constexpr
void write_range_as_bytes(Target* dest, SrcIter src_begin, SrcSentinel src_end) noexcept {
	using SrcValue = std::iter_value_t<SrcIter>;
	const auto walk_n_copy = [&] {
		for (auto i = 0zu; src_begin != src_end; ++src_begin, ++i) {
			write_obj_as_bytes(dest + sizeof(SrcValue) * i, *src_begin);
		}
	};
	if consteval {
		walk_n_copy();
	}
	else {
		if constexpr (std::contiguous_iterator<SrcIter>
			&& std::sized_sentinel_for<SrcSentinel, SrcIter>
		) {
			std::memcpy(dest, std::to_address(src_begin),
				sizeof(SrcValue) * static_cast<size_t>(std::ranges::distance(src_begin, src_end)));
		}
		else {
			walk_n_copy();
		}
	}
}

} // namespace fr
#endif // include guard
