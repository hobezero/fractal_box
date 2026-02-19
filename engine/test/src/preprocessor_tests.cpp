#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/preprocessor.hpp"

TEST_CASE("FR_CONDITIONAL", "[u][engine][core][preprocessor]") {
	CHECK(FR_CONDITIONAL(0, 22, 33) == 33);
	CHECK(FR_CONDITIONAL(1, 22, 33) == 22);
}

TEST_CASE("FR_IS_VA_EMPTY", "[u][engine][core][preprocessor]") {
	CHECK(FR_IS_VA_EMPTY() == 1);
	CHECK(FR_IS_VA_EMPTY(30) == 0);
	CHECK(FR_IS_VA_EMPTY('a') == 0);
	CHECK(FR_IS_VA_EMPTY(90, 'b') == 0);
	CHECK(FR_IS_VA_EMPTY(53, 'c', 2.) == 0);
}

TEST_CASE("FR_VA_SIZE_NON_EMPTY", "[u][engine][core][preprocessor]") {
	CHECK(FR_VA_SIZE_NON_EMPTY('a') == 1);
	CHECK(FR_VA_SIZE_NON_EMPTY('a', b) == 2);
	CHECK(FR_VA_SIZE_NON_EMPTY(5, "sdf", 7) == 3);
	CHECK(FR_VA_SIZE_NON_EMPTY('5', 6, "7", 8.2f, 9) == 5);
}

TEST_CASE("FR_VA_SIZE", "[u][engine][core][preprocessor]") {
	CHECK(FR_VA_SIZE() == 0);
	CHECK(FR_VA_SIZE('a') == 1);
	CHECK(FR_VA_SIZE('a', "bar") == 2);
	CHECK(FR_VA_SIZE(5, "foo", 7) == 3);
	CHECK(FR_VA_SIZE('5', 6, "7", 8.2f, 9) == 5);
}

#define FR_TEST_ADD_0() 0
#define FR_TEST_ADD_1(a) (a)
#define FR_TEST_ADD_2(a, b) ((a) + (b))
#define FR_TEST_ADD_3(a, b, c) ((a) + (b) + (c))

#define FR_TEST_ADD(...) FR_OVERLOAD_MACRO(FR_TEST_ADD_, __VA_ARGS__)(__VA_ARGS__)

TEST_CASE("FR_OVERLOAD_MACRO", "[u][engine][core][preprocessor]") {
	CHECK(FR_TEST_ADD() == 0);
	CHECK(FR_TEST_ADD(42) == 42);
	CHECK(FR_TEST_ADD(11, 22) == 33);
	CHECK(FR_TEST_ADD(33, 11, 22) == 66);
}
