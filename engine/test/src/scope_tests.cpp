#include "fractal_box/core/scope.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ScopeExit", "[u][engine][core][scope]") {
	auto count = 0;
	const auto inc_count = [&] { ++count; };

	SECTION("default use case") {
		{
			const auto a = fr::ScopeExit{inc_count};
			{
				const auto b = fr::ScopeExit{inc_count};
				const auto c = fr::ScopeExit{inc_count};
				CHECK(count == 0);
			}
			CHECK(count == 2);
		}
		CHECK(count == 3);
	}
	SECTION("release()") {
		{
			auto a = fr::ScopeExit{inc_count};
			a.release();
			CHECK(count == 0);
		}
		CHECK(count == 0);
	}
	SECTION("execute()") {
		{
			auto a = fr::ScopeExit{inc_count};
			a.execute();
			CHECK(count == 1);
		}
		CHECK(count == 1);
	}
}

TEST_CASE("FR_DEFER", "[u][engine][core][scope]") {
	auto count = 0;
	const auto inc_count = [&] { ++count; };

	SECTION("default use case") {
		{
			FR_DEFER [&] { ++count; };
			{
				FR_DEFER inc_count;
				FR_DEFER inc_count;
				CHECK(count == 0);
			}
			CHECK(count == 2);
		}
		CHECK(count == 3);
	}
}
