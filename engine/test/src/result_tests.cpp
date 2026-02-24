#include "fractal_box/core/result.hpp"

#include <string>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Result", "[u][engine][core][result]") {
	SECTION("type properties") {
		STATIC_CHECK(std::is_default_constructible_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_default_constructible_v<fr::Result<int, std::string, int>>);

		STATIC_CHECK(std::is_copy_constructible_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_copy_constructible_v<fr::Result<int, unsigned>>);
		STATIC_CHECK_FALSE(std::is_nothrow_copy_constructible_v<fr::Result<int, std::string, int>>);

		STATIC_CHECK(std::is_copy_assignable_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK_FALSE(std::is_nothrow_copy_assignable_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_copy_assignable_v<fr::Result<int, unsigned>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::Result<int, std::string, int>>);

		STATIC_CHECK(std::is_move_assignable_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::Result<int, std::string, int>>);

		STATIC_CHECK(std::is_destructible_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_nothrow_destructible_v<fr::Result<int, std::string, int>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::Result<int, unsigned>>);
	}
	SECTION("default-construct") {
		SECTION("void value") {
			auto a = fr::Result<void, std::string, int>{};
			REQUIRE(a.has_value());
		}
		SECTION("non-void value") {
			auto a = fr::Result<int, std::string, int>{};
			REQUIRE(a.has_value());
			CHECK(a.value() == 0);
		}
	}
	SECTION("construct from value") {
		auto a = fr::Result<int, std::string, int>{23};
		REQUIRE(a.has_value());
		CHECK(a.value() == 23);
	}
	SECTION("construct from error (from_error_as)") {
		SECTION("void value") {
			auto a = fr::Result<void, std::string, int>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");

			auto b = fr::Result<void, std::string>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(b.has_value());
			REQUIRE(b.has_error<std::string>());
			CHECK(b.error<std::string>() == "abcdef");
		}
		SECTION("non-void value") {
			auto a = fr::Result<int, std::string, int>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");

			auto b = fr::Result<int, std::string>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(b.has_value());
			REQUIRE(b.has_error<std::string>());
			CHECK(b.error<std::string>() == "abcdef");
		}
	}
	SECTION("construct from error (from_error)") {
		SECTION("void value") {
			auto a = fr::Result<void, std::string>{fr::from_error, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");
		}
		SECTION("non-void value") {
			auto a = fr::Result<int, std::string>{fr::from_error, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			REQUIRE(a.has_error());
			CHECK(a.error<std::string>() == "abcdef");
		}
	}
}
