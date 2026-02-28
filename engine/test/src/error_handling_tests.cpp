#include "fractal_box/core/error_handling/result.hpp"
#include "fractal_box/core/error_handling/status.hpp"

#include <string>

#include <catch2/catch_test_macros.hpp>

namespace {

struct A { };

struct HardConstructable {
	HardConstructable() = delete;

	constexpr
	HardConstructable(A) noexcept { }
};

} // namespace

TEST_CASE("Result", "[u][engine][core][error_handling]") {
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
			const auto a = fr::Result<void, std::string, int>{};
			REQUIRE(a.has_value());
		}
		SECTION("non-void value") {
			const auto a = fr::Result<int, std::string, int>{};
			REQUIRE(a.has_value());
			CHECK(a.value() == 0);
			CHECK(*a == 0);
			CHECK(*a.operator->() == 0);
		}
	}
	SECTION("construct from value") {
		const auto a = fr::Result<int, std::string, int>{23};
		REQUIRE(a.has_value());
		CHECK(a.value() == 23);
		CHECK(*a == 23);
		CHECK(*a.operator->() == 23);
	}
	SECTION("construct from value in-place") {
		const auto a = fr::Result<int, std::string, int>{fr::in_place, 23};
		REQUIRE(a.has_value());
		CHECK(a.value() == 23);
		CHECK(*a == 23);
		CHECK(*a.operator->() == 23);
	}
	SECTION("construct from error (from_error_as)") {
		SECTION("void value") {
			const auto a = fr::Result<void, std::string, int>{fr::from_error_as<std::string>,
				"abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");

			const auto b = fr::Result<void, std::string>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(b.has_value());
			REQUIRE(b.has_error<std::string>());
			CHECK(b.error<std::string>() == "abcdef");
		}
		SECTION("non-void value") {
			const auto a = fr::Result<int, std::string, int>{fr::from_error_as<std::string>,
				"abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");

			const auto b = fr::Result<int, std::string>{fr::from_error_as<std::string>, "abcdef"};
			CHECK_FALSE(b.has_value());
			REQUIRE(b.has_error<std::string>());
			CHECK(b.error<std::string>() == "abcdef");
		}
	}
	SECTION("construct from error (from_error)") {
		SECTION("void value") {
			const auto a = fr::Result<void, std::string>{fr::from_error, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			CHECK(a.error<std::string>() == "abcdef");
		}
		SECTION("non-void value") {
			const auto a = fr::Result<int, std::string>{fr::from_error, "abcdef"};
			CHECK_FALSE(a.has_value());
			REQUIRE(a.has_error<std::string>());
			REQUIRE(a.has_error());
			CHECK(a.error<std::string>() == "abcdef");
		}
	}
	SECTION("construct from error subset") {
		SECTION("const version") {
			SECTION("void value") {
				const auto a = fr::Result<void, std::string, int>{fr::from_error_as<int>, 23};
				const auto b = fr::Result<A, std::string, unsigned, int>{fr::from_error, a};
				REQUIRE(b.has_error<int>());
				CHECK(b.error<int>() == 23);
			}
			SECTION("non-void value") {
				const auto a = fr::Result<std::string, std::string, int>{fr::from_error_as<int>,
					23};
				const auto b = fr::Result<A, std::string, unsigned, int>{fr::from_error, a};
				REQUIRE(b.has_error<int>());
				CHECK(b.error<int>() == 23);
			}
		}
		SECTION("moved-out version") {
			SECTION("void value") {
				auto a = fr::Result<void, std::string, int>{fr::from_error_as<std::string>, "ERR"};
				auto b = fr::Result<A, std::string, unsigned, int>{fr::from_error, std::move(a)};
				REQUIRE(b.has_error<std::string>());
				CHECK(b.error<std::string>() == "ERR");
				CHECK(a.error<std::string>().empty());
			}
			SECTION("non-void value") {
				auto a = fr::Result<int, std::string, int>{fr::from_error_as<std::string>, "ERR"};
				auto b = fr::Result<A, std::string, unsigned, int>{fr::from_error, std::move(a)};
				REQUIRE(b.has_error<std::string>());
				CHECK(b.error<std::string>() == "ERR");
				CHECK(a.error<std::string>().empty());
			}
		}
	}
	SECTION("assign value") {
		auto a = fr::Result<std::string, int>{fr::from_error, -1};
		CHECK_FALSE(a.has_value());
		a = "abcdef";
		REQUIRE(a.has_value());
		CHECK(a.value() == "abcdef");
	}
	SECTION("emplace value") {
		auto a = fr::Result<int, std::string>{fr::from_error, "ERR"};
		CHECK_FALSE(a.has_value());
		a.emplace(3);
		REQUIRE(a.has_value());
		CHECK(a.value() == 3);
	}
	SECTION("value_or") {
		SECTION("const version") {
			const auto a = fr::Result<std::string, int>{"abcdef"};
			CHECK(a.value_or("qwerty") == "abcdef");

			const auto b = fr::Result<std::string, int>{fr::from_error, 34};
			CHECK(b.value_or("qwerty") == "qwerty");
		}
		SECTION("moved-out version") {
			auto a = fr::Result<std::string, int>{"abcdef"};
			CHECK(std::move(a).value_or("qwerty") == "abcdef");

			auto b = fr::Result<std::string, int>{fr::from_error, 34};
			CHECK(std::move(b).value_or("qwerty") == "qwerty");
		}
	}
	SECTION("error_or") {
		SECTION("const version") {
			const auto a = fr::Result<int, std::string>{fr::from_error, "ABC"};
			CHECK(a.error_or("DEF") == "ABC");

			const auto b = fr::Result<int, std::string>{23};
			CHECK(b.error_or("DEF") == "DEF");

			const auto c = fr::Result<int, int, unsigned, std::string>{
				fr::from_error_as<std::string>, "ABC"};
			CHECK(c.error_or<std::string>("DEF") == "ABC");

			const auto d = fr::Result<int, int, unsigned, std::string>{
				fr::from_error_as<unsigned>, 24u};
			CHECK(d.error_or<std::string>("DEF") == "DEF");

			const auto e = fr::Result<int, int, unsigned, std::string>{23};
			CHECK(e.error_or<std::string>("DEF") == "DEF");
		}
		SECTION("moved-out version") {
			auto a = fr::Result<int, std::string>{fr::from_error, "ABC"};
			CHECK(std::move(a).error_or("DEF") == "ABC");

			auto b = fr::Result<int, std::string>{23};
			CHECK(std::move(b).error_or("DEF") == "DEF");

			auto c = fr::Result<int, int, unsigned, std::string>{
				fr::from_error_as<std::string>, "ABC"};
			CHECK(std::move(c).error_or<std::string>("DEF") == "ABC");

			auto d = fr::Result<int, int, unsigned, std::string>{
				fr::from_error_as<unsigned>, 24u};
			CHECK(std::move(d).error_or<std::string>("DEF") == "DEF");

			auto e = fr::Result<int, int, unsigned, std::string>{23};
			CHECK(std::move(e).error_or<std::string>("DEF") == "DEF");
		}
	}
}

TEST_CASE("Status", "[u][engine][core][error_handling]") {
	SECTION("type properties") {
		STATIC_CHECK(std::is_default_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_default_constructible_v<fr::Status<>>);
		STATIC_CHECK(std::is_nothrow_default_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_default_constructible_v<fr::Status<>>);
		STATIC_CHECK_FALSE(std::is_default_constructible_v<fr::Status<HardConstructable>>);

		STATIC_CHECK(std::is_copy_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_copy_constructible_v<fr::Status<int>>);
		STATIC_CHECK(std::is_copy_constructible_v<fr::Status<>>);
		STATIC_CHECK_FALSE(std::is_nothrow_copy_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_copy_constructible_v<fr::Status<int>>);
		STATIC_CHECK(std::is_nothrow_copy_constructible_v<fr::Status<>>);

		STATIC_CHECK(std::is_copy_assignable_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_copy_assignable_v<fr::Status<int>>);
		STATIC_CHECK(std::is_copy_assignable_v<fr::Status<>>);
		STATIC_CHECK_FALSE(std::is_nothrow_copy_assignable_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_copy_assignable_v<fr::Status<int>>);
		STATIC_CHECK(std::is_nothrow_copy_assignable_v<fr::Status<>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_move_constructible_v<fr::Status<>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::Status<>>);

		STATIC_CHECK(std::is_move_assignable_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::Status<>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::Status<>>);

		STATIC_CHECK(std::is_destructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_nothrow_destructible_v<fr::Status<std::string>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::Status<int>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::Status<>>);
	}
	SECTION("default-construct") {
		SECTION("void value") {
			const auto a = fr::Status<>{};
			CHECK(a.has_value());
			CHECK(a);
		}
		SECTION("non-void value") {
			const auto a = fr::Status<std::string>{};
			REQUIRE(a.has_value());
			REQUIRE(a);
			CHECK(a.value() == std::string{}); // NOLINT
			CHECK(*a == std::string{}); // NOLINT
			CHECK(*a.operator->() == std::string{}); // NOLINT
		}
	}
	SECTION("construct from value") {
		const auto a = fr::Status<std::string>{"abcd"};
		REQUIRE(a.has_value());
		REQUIRE(a);
		CHECK(a.value() == "abcd");
		CHECK(*a == "abcd");
		CHECK(*a.operator->() == "abcd");
	}
	SECTION("construct from value in-place") {
		const auto a = fr::Status<std::string>{fr::in_place, "abcd"};
		REQUIRE(a.has_value());
		REQUIRE(a);
		CHECK(a.value() == "abcd");
		CHECK(*a == "abcd");
		CHECK(*a.operator->() == "abcd");
	}
	SECTION("construct from error") {
		SECTION("void value") {
			const auto a = fr::Status<>{fr::from_error};
			CHECK_FALSE(a.has_value());
			CHECK_FALSE(a);
		}
		SECTION("non-void value") {
			const auto a = fr::Status<std::string>{fr::from_error};
			CHECK_FALSE(a.has_value());
			CHECK_FALSE(a);
		}
	}
	SECTION("assign value") {
		auto a = fr::Status<std::string>{fr::from_error};
		a = "abcd";
		CHECK(a.value() == "abcd");

		auto b = fr::Status<std::string>{"qwe"};
		b = "abcd";
		CHECK(b.value() == "abcd");

	}
	SECTION("emplace value") {
		auto a = fr::Status<std::string>{fr::from_error};
		a.emplace("abcd");
		CHECK(a.value() == "abcd");

		auto b = fr::Status<std::string>{"qwe"};
		b.emplace("abcd");
		CHECK(b.value() == "abcd");
	}
}
