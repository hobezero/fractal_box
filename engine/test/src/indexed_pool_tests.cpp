#include "fractal_box/core/containers/indexed_pool.hpp"

#include <string>

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>

#include "test_common/test_helpers.hpp"

namespace {

struct Foo {
	[[maybe_unused]] friend
	auto operator==(const Foo&, const Foo&) -> bool = default;

public:
	int x;
	std::string text;
};


} // namespace

template<>
struct fmt::formatter<Foo>: formatter<char> {
	auto format(const Foo& foo, format_context& ctx) const {
		return fmt::format_to(ctx.out(), "{{{}, {}}}", foo.x, foo.text);
	}
};

template<>
struct Catch::StringMaker<Foo> {
	static
	auto convert(const Foo& foo) -> std::string {
		return fmt::format("{}", foo);
	}
};

static
auto check_pool_value(auto& pool, size_t idx, const auto& expected_value) {
	INFO(fmt::format("expecting value at idx = {} to be '{}'", idx, expected_value));
	CHECK(pool.contains(idx));
	REQUIRE(pool.try_get(idx));
	CHECK(*pool.try_get(idx) == expected_value);
	CHECK(pool.get(idx) == expected_value);
	CHECK(pool.unsafe_get(idx) == expected_value);
}

TEST_CASE("IndexedPool", "[u][engine][core][containers]") {
	using Pool = fr::IndexedPool<Foo>;

	SECTION("type properties") {
		STATIC_CHECK(std::is_default_constructible_v<Pool>);
		STATIC_CHECK(std::is_copy_constructible_v<Pool>);
		STATIC_CHECK(std::is_copy_assignable_v<Pool>);
		STATIC_CHECK(std::is_move_constructible_v<Pool>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<Pool>);
		STATIC_CHECK(std::is_move_assignable_v<Pool>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<Pool>);
		STATIC_CHECK(std::is_destructible_v<Pool>);
	}

	const auto a_expected = Foo{23, "abc"};
	const auto b_expected = Foo{0, frt::lorem_text};
	const auto c_expected = Foo{-84, ""};
	const auto d_expected = Foo{123456, "Nananananananana"};
	const auto e_expected = Foo{555, "elephant"};
	const auto f_expected = Foo{777, "foo"};

	auto pool = fr::IndexedPool<Foo>{};

	INFO("0. empty state");
	CHECK(pool.size() == 0);
	CHECK(pool.empty());
	CHECK_FALSE(pool.try_get(0));
	CHECK_FALSE(pool.try_get(1));

	// IIFE's make sure InsertResult's gets destroyed before the next insertion which would
	// invalidate `InsertResult::value()` reference
	INFO("1. emplace 'a'");
	const auto a_idx = [&] {
		const auto res = pool.emplace(a_expected.x, a_expected.text);
		CHECK(res.value() == a_expected);
		check_pool_value(pool, res.index(), a_expected);
		CHECK(pool.size() == 1);
		CHECK_FALSE(pool.empty());
		return res.index();
	}();

	INFO("2. insert 'b'");
	const auto b_idx = [&] {
		const auto res = pool.insert(b_expected);
		CHECK(res.value() == b_expected);
		check_pool_value(pool, a_idx, a_expected);
		check_pool_value(pool, res.index(), b_expected);
		CHECK(pool.size() == 2);
		return res.index();
	}();

	INFO("3. emplace 'c'");
	const auto c_idx = [&] {
		const auto res = pool.insert(c_expected);
		CHECK(res.value() == c_expected);
		check_pool_value(pool, a_idx, a_expected);
		check_pool_value(pool, b_idx, b_expected);
		check_pool_value(pool, res.index(), c_expected);
		CHECK(pool.size() == 3);
		return res.index();
	}();

	INFO("4. erase 'b'");
	{
		CHECK(pool.erase(b_idx) == 1);
		CHECK_FALSE(pool.contains(b_idx));
		check_pool_value(pool, a_idx, a_expected);
		check_pool_value(pool, c_idx, c_expected);
		CHECK(pool.size() == 2);
	}

	INFO("5. erase 'b' (second time)");
	{
		CHECK(pool.erase(b_idx) == 0);
		CHECK_FALSE(pool.contains(b_idx));
		check_pool_value(pool, a_idx, a_expected);
		check_pool_value(pool, c_idx, c_expected);
		CHECK(pool.size() == 2);
	}

	INFO("6. unsafe_erase 'a'");
	{
		pool.unsafe_erase(a_idx);
		CHECK_FALSE(pool.contains(a_idx));
		check_pool_value(pool, c_idx, c_expected);
		CHECK(pool.size() == 1);
	}

	INFO("7. insert 'd'");
	const auto d_idx = [&] {
		const auto res = pool.insert(d_expected);
		CHECK(res.value() == d_expected);
		check_pool_value(pool, c_idx, c_expected);
		check_pool_value(pool, res.index(), d_expected);
		CHECK(pool.size() == 2);
		return res.index();
	}();

	INFO("8. insert 'e'");
	const auto e_idx = [&] {
		const auto res = pool.insert(e_expected);
		CHECK(res.value() == e_expected);
		check_pool_value(pool, c_idx, c_expected);
		check_pool_value(pool, d_idx, d_expected);
		check_pool_value(pool, res.index(), e_expected);
		CHECK(pool.size() == 3);
		return res.index();
	}();

	INFO("9. emplace 'f'");
	const auto f_idx = [&] {
		const auto res = pool.emplace(f_expected.x, f_expected.text);
		CHECK(res.value() == f_expected);
		check_pool_value(pool, c_idx, c_expected);
		check_pool_value(pool, d_idx, d_expected);
		check_pool_value(pool, e_idx, e_expected);
		check_pool_value(pool, res.index(), f_expected);
		CHECK(pool.size() == 4);
		return res.index();
	}();

	INFO("10. erase out-of-bounds");
	{
		CHECK(pool.erase(999'999'999'999) == 0);
	}

	SECTION("11. clear") {
		pool.clear();

		CHECK(pool.size() == 0);
		CHECK_FALSE(pool.contains(a_idx));
		CHECK_FALSE(pool.contains(b_idx));
		CHECK_FALSE(pool.contains(c_idx));
		CHECK_FALSE(pool.contains(d_idx));
		CHECK_FALSE(pool.contains(e_idx));
		CHECK_FALSE(pool.contains(f_idx));
	}
	SECTION("11. erase all one by one") {
		pool.erase(a_idx);
		pool.erase(b_idx);
		pool.erase(c_idx);
		pool.erase(d_idx);
		pool.erase(e_idx);
		pool.erase(f_idx);

		CHECK(pool.size() == 0);
		CHECK_FALSE(pool.contains(a_idx));
		CHECK_FALSE(pool.contains(b_idx));
		CHECK_FALSE(pool.contains(c_idx));
		CHECK_FALSE(pool.contains(d_idx));
		CHECK_FALSE(pool.contains(e_idx));
		CHECK_FALSE(pool.contains(f_idx));
	}
}
