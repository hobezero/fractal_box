#include "fractal_box/core/containers/linear_flat_set.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("LinearFlatSet", "[u][engine][core][containers]") {
	using IntSet = fr::LinearFlatSet<int>;

	SECTION("type properties") {
		STATIC_CHECK(std::is_default_constructible_v<IntSet>);
		STATIC_CHECK(std::is_copy_constructible_v<IntSet>);
		STATIC_CHECK(std::is_copy_assignable_v<IntSet>);
		STATIC_CHECK(std::is_move_constructible_v<IntSet>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<IntSet>);
		STATIC_CHECK(std::is_move_assignable_v<IntSet>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<IntSet>);
		STATIC_CHECK(std::is_destructible_v<IntSet>);
	}

	auto set = IntSet{};
	SECTION("initialized state is empty") {
		CHECK(set.empty());
		CHECK(set.size() == 0); // NOLINT
		CHECK(!set.contains(5));
		CHECK(!set.contains(0));
		CHECK(set.find(9) == set.end());
		CHECK(std::as_const(set).find(9) == set.cend());
	}
	SECTION("insert-emplace") {
		const auto ten = set.insert(10);
		REQUIRE(ten.success);
		CHECK(*ten.where == 10);
		CHECK(set.size() == 1);
		CHECK(!set.empty());

		const auto twenty = set.emplace(20);
		CHECK(twenty.success);
		CHECK(*twenty.where == 20);
		CHECK(set.size() == 2);
		CHECK(!set.empty());

		const auto four = set.insert(4);
		REQUIRE(four.success);
		CHECK(*four.where == 4);
		CHECK(set.size() == 3);
		CHECK(!set.empty());

		SECTION("add duplicate") {
			const auto doubleFour = set.insert(4);
			CHECK(!doubleFour.success);
			CHECK(doubleFour.where == four.where);
			CHECK(*doubleFour.where == 4);
			CHECK(set.size() == 3);
			CHECK(!set.empty());

			const auto tripleFour = set.emplace(4);
			CHECK(!tripleFour.success);
			CHECK(tripleFour.where == four.where);
			CHECK(*tripleFour.where == 4);
			CHECK(set.size() == 3);
			CHECK(!set.empty());
		}
		SECTION("lookup") {
			CHECK(set.contains(10));
			CHECK(!set.contains(35));
			CHECK(set.find(4) == four.where);
			CHECK(set.find(35) == set.end());
		}
		SECTION("erase") {
			CHECK(set.erase(4) == 1);
			CHECK(set.size() == 2);
			CHECK(!set.empty());
			CHECK(!set.contains(4));
			CHECK(set.contains(20));

			SECTION("double erase") {
				CHECK(set.erase(4) == 0);
				CHECK(set.size() == 2);
				CHECK(!set.empty());
				CHECK(!set.contains(4));
				CHECK(set.contains(20));
			}
		}
		SECTION("swap") {
			CHECK(std::is_nothrow_swappable_v<IntSet>);
			IntSet set2;
			set2.insert(56);
			swap(set, set2);
			CHECK(set2.size() == 3);
			CHECK(set.size() == 1);
			CHECK(set2.contains(4));
			CHECK(set.contains(56));
		}
	}
}
