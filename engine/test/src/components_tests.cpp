#include <catch2/catch_test_macros.hpp>

#include <glm/gtx/matrix_transform_2d.hpp>

#include "fractal_box/components/transform.hpp"

TEST_CASE("Transform", "[u][engine][components]") {
	SECTION("type properties") {
		CHECK(std::is_default_constructible_v<fr::Transform>);
		CHECK(std::is_nothrow_default_constructible_v<fr::Transform>);
		CHECK(std::is_copy_constructible_v<fr::Transform>);
		CHECK(std::is_nothrow_copy_constructible_v<fr::Transform>);
		CHECK(std::is_copy_assignable_v<fr::Transform>);
		CHECK(std::is_nothrow_copy_assignable_v<fr::Transform>);
		CHECK(std::is_move_constructible_v<fr::Transform>);
		CHECK(std::is_nothrow_move_constructible_v<fr::Transform>);
		CHECK(std::is_move_assignable_v<fr::Transform>);
		CHECK(std::is_nothrow_move_assignable_v<fr::Transform>);
		CHECK(std::is_destructible_v<fr::Transform>);
		CHECK(std::is_nothrow_destructible_v<fr::Transform>);
	}

	SECTION("triviality") {
		CHECK(std::is_trivially_copyable_v<fr::Transform>);
		CHECK(std::is_trivially_copy_constructible_v<fr::Transform>);
		CHECK(std::is_trivially_move_constructible_v<fr::Transform>);
		CHECK(std::is_trivially_copy_assignable_v<fr::Transform>);
		CHECK(std::is_trivially_move_assignable_v<fr::Transform>);
		CHECK(std::is_trivially_destructible_v<fr::Transform>);
	}
}
