#include "fractal_box/core/bit_manip.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("bitmask", "[u][engine][core][bit_manip]") {
	SECTION("all zeroes") {
		CHECK(fr::bitmask<uint8_t, 0, 0> == uint8_t{0});
		CHECK(fr::bitmask<int8_t, 0, 0> == int8_t{0});

		CHECK(fr::bitmask<uint16_t, 0, 0> == uint16_t{0});
		CHECK(fr::bitmask<int16_t, 0, 0> == int16_t{0});

		CHECK(fr::bitmask<uint32_t, 0, 0> == uint32_t{0});
		CHECK(fr::bitmask<int32_t, 0, 0> == int32_t{0});

		CHECK(fr::bitmask<uint64_t, 0, 0> == uint64_t{0});
		CHECK(fr::bitmask<int64_t, 0, 0> == int64_t{0});
	}
	SECTION("all ones") {
		CHECK(fr::bitmask<uint8_t, 0, 8> == uint8_t{0xFF});
		CHECK(fr::bitmask<int8_t, 0, 8> == std::bit_cast<int8_t>(uint8_t{0xFF}));

		CHECK(fr::bitmask<uint16_t, 0, 16> == uint16_t{0xFFFF});
		CHECK(fr::bitmask<int16_t, 0, 16> == std::bit_cast<int16_t>(uint16_t{0xFFFF}));

		CHECK(fr::bitmask<uint32_t, 0, 32> == UINT32_C(0xFFFF'FFFF));
		CHECK(fr::bitmask<int32_t, 0, 32> == std::bit_cast<int32_t>(UINT32_C(0xFFFF'FFFF)));

		CHECK(fr::bitmask<uint64_t, 0, 64> == UINT64_C(0xFFFF'FFFF'FFFF'FFFF));
		CHECK(fr::bitmask<int64_t, 0, 64> == std::bit_cast<int64_t>(UINT64_C(
			0xFFFF'FFFF'FFFF'FFFF)));
	}

	CHECK(fr::bitmask<uint8_t, 0, 3> == uint8_t{0xE0});

	CHECK(fr::bitmask<uint16_t, 1, 2> == uint16_t{0x6000});
	CHECK(fr::bitmask<uint16_t, 10, 6> == uint16_t{0x003F});

	CHECK(fr::bitmask<uint32_t, 5, 20> == UINT32_C(0x07FF'FF80));
	CHECK(fr::bitmask<uint32_t, 6, 1> == UINT32_C(0x0200'0000));
	CHECK(fr::bitmask<uint32_t, 0, 7> == UINT32_C(0xFE00'0000));

	CHECK(fr::bitmask<uint64_t, 10, 45> == UINT64_C(0x003F'FFFF'FFFF'FE00));
	CHECK(fr::bitmask<uint64_t, 0, 58> == UINT64_C(0xFFFF'FFFF'FFFF'FFC0));
}
