#ifndef FR_TEST_TEST_COMMON_HASHING_FMT_HPP
#define FR_TEST_TEST_COMMON_HASHING_FMT_HPP

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "fractal_box/core/hashing/uni_hasher.hpp"

template<>
struct Catch::StringMaker<fr::detail::UniHashableLens1> {
	static
	auto convert(const fr::detail::UniHashableLens1& lens) -> std::string {
		return fmt::format("{{{}, {}}}", lens.path, lens.byte_size);
	}
};

#endif // include guard
