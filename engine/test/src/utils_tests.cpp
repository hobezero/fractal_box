#include <span>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/enum_utils.hpp"

namespace {

struct SwapMock {
	int x;
	int swapCount = 0;

	friend
	void swap(SwapMock& lhs, SwapMock& rhs) noexcept {
		std::swap(lhs.x, rhs.x);
		++lhs.swapCount;
		++rhs.swapCount;
	}

	[[maybe_unused]] friend constexpr
	auto operator==(SwapMock lhs, SwapMock rhs) noexcept -> bool {
		return lhs.x == rhs.x;
	}
};

} // namespace

TEST_CASE("WithDefault", "[u][engine][core][utils]") {
	SECTION("type properties") {
		using SimpleWithDefault = fr::WithDefault<int, 1>;

		CHECK(std::regular<SimpleWithDefault>);

		SECTION("default constructor") {
			STATIC_REQUIRE(std::is_default_constructible_v<SimpleWithDefault>);
			STATIC_CHECK_FALSE(std::is_trivially_default_constructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_default_constructible_v<SimpleWithDefault>);
		}
		SECTION("copy constructor") {
			STATIC_REQUIRE(std::is_copy_constructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_trivially_copy_constructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_copy_constructible_v<SimpleWithDefault>);
		}
		SECTION("copy assignment") {
			STATIC_REQUIRE(std::is_copy_assignable_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_trivially_copy_assignable_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_copy_assignable_v<SimpleWithDefault>);
		}
		SECTION("move constructor") {
			STATIC_REQUIRE(std::is_move_constructible_v<SimpleWithDefault>);
			STATIC_CHECK_FALSE(std::is_trivially_move_constructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_move_constructible_v<SimpleWithDefault>);
		}
		SECTION("move assignment") {
			STATIC_REQUIRE(std::is_move_assignable_v<SimpleWithDefault>);
			STATIC_CHECK_FALSE(std::is_trivially_move_assignable_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_move_assignable_v<SimpleWithDefault>);
		}
		SECTION("destructor") {
			STATIC_REQUIRE(std::is_destructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_trivially_destructible_v<SimpleWithDefault>);
			STATIC_CHECK(std::is_nothrow_destructible_v<SimpleWithDefault>);
		}
	}
	SECTION("default constructed WithDefault has the default value") {
		fr::WithDefault<int, 4> w;
		CHECK(w.value() == 4);
		CHECK(*w == 4);
	}
	SECTION("custom initial value") {
		auto w = fr::WithDefault<int, 4>{42};
		CHECK(w.value() == 42);
		CHECK(*w == 42);
	}
	SECTION("user-defined types are supported") {
		struct MyData {
			int x;
			float y;
		};

		fr::WithDefault<MyData, MyData{.x = 5, .y = 3.1f}> w;
		CHECK(w->x == 5);
		CHECK(w->y == 3.1f);
	}
	SECTION("copying") {
		constexpr auto do_test = []<int FirstDefault, int SecondDefault>() {
			const fr::WithDefault<int, FirstDefault> a = 42;

			fr::WithDefault<int, SecondDefault> constructed{a};
			CHECK(constructed.value() == 42);

			fr::WithDefault<int, SecondDefault> assigned;
			assigned = a;
			CHECK(assigned.value() == 42);
		};

		SECTION("the same default value") {
			do_test.template operator()<4, 4>();
		}
		SECTION("different default value") {
			do_test.template operator()<4, 9>();
		}
	}
	SECTION("moving") {
		constexpr auto do_test = []<int FirstDefault, int SecondDefault>() {
			{
				fr::WithDefault<int, FirstDefault> a = 42;
				fr::WithDefault<int, SecondDefault> constructed = std::move(a);
				CHECK(constructed.value() == 42);
				CHECK(a.value() == FirstDefault);
			}
			{
				fr::WithDefault<int, FirstDefault> a = 42;
				fr::WithDefault<int, SecondDefault> assigned;
				assigned = std::move(a);
				CHECK(assigned.value() == 42);
				CHECK(a.value() == FirstDefault);
			}
		};

		SECTION("the same default value") {
			do_test.template operator()<4, 4>();
		}
		SECTION("different default values") {
			do_test.template operator()<4, 9>();
		}
	}

	SECTION("emplace value") {
		fr::WithDefault<int, -1> a = 42;
		a.emplace(1);
		CHECK(a.value() == 1);
		(a = 2) = 3;
		CHECK(a.value() == 3);
	}

	SECTION("release") {
		fr::WithDefault<int, 4> a = 42;
		CHECK(a.release() == 42);
		CHECK(a.value() == 4);
	}

	constexpr auto do_comparison_test = []<class A, class B> {
		CHECK(A{2} == B{2});
		CHECK_FALSE(A{2} == B{3});

		CHECK(A{2} != B{3});
		CHECK_FALSE(A{2} != B{2});

		CHECK(A{2} < B{3});
		CHECK_FALSE(A{2} < B{2});

		CHECK(A{2} <= B{3});
		CHECK(A{2} <= B{2});
		CHECK_FALSE(A{2} <= B{1});

		CHECK(A{2} > B{1});
		CHECK_FALSE(A{2} > B{2});

		CHECK(A{2} >= B{1});
		CHECK(A{2} >= B{2});
		CHECK_FALSE(A{2} >= B{3});
	};
	SECTION("compare with the same WithDefault type") {
		do_comparison_test.template operator()<
			fr::WithDefault<int, 4>,
			fr::WithDefault<int, 4>
		>();
	}
	SECTION("compare with a different WithDefault type") {
		do_comparison_test.template operator()<
			fr::WithDefault<int, 4>,
			fr::WithDefault<int, 9>
		>();
	}
	SECTION("compare with `T`") {
		do_comparison_test.template operator()<
			fr::WithDefault<int, 4>,
			int
		>();
	}
	SECTION("swap") {
		constexpr auto do_test = []<int FirstDefault, int SecondDefault>() {
			fr::WithDefaultValue<SwapMock{FirstDefault}> a = SwapMock{31};
			fr::WithDefaultValue<SwapMock{SecondDefault}> b = SwapMock{42};

			swap(a, b);
			CHECK(a->x == 42);
			CHECK(b->x == 31);
			CHECK(a->swapCount == 1);
			CHECK(b->swapCount == 1);

			a.swap(b);
			CHECK(a->x == 31);
			CHECK(b->x == 42);
			CHECK(a->swapCount == 2);
			CHECK(b->swapCount == 2);

			// Make sure that our swap doesn't cause ambiguity when `std::swap` is visible
			using std::swap;
			swap(a, b);
			CHECK(a->x == 42);
			CHECK(b->x == 31);
			CHECK(a->swapCount == 3);
			CHECK(b->swapCount == 3);

			if constexpr (FirstDefault == SecondDefault) {
				std::swap(a, b);
				CHECK(a->swapCount == 3);
				CHECK(b->swapCount == 3);
			}
		};

		SECTION("the same default value") {
			do_test.template operator()<4, 4>();
		}
		SECTION("different default values") {
			do_test.template operator()<4, 9>();
		}
	}
}

namespace {

enum class MyFlag: unsigned {
	A = 1 << 0,
	B = 1 << 1,
	C = 1 << 2,
	D = 1 << 3,
};

} // namespace

using MyFlags = fr::Flags<MyFlag>;

template<>
struct Catch::StringMaker<MyFlags> {
	static std::string convert(const MyFlags& flags) {
		auto append_list = [is_first = true] (
			std::string& result, std::string_view tail, std::string_view delim
		) mutable {
			if (!is_first) {
				result.reserve(result.size() + delim.size() + tail.size());
				result.append(delim);
			}
			is_first = false;
			result.append(tail);
		};

		static constexpr std::string_view delim = "|";
		std::string result = "{";
		if ((flags.raw_value() & std::to_underlying(MyFlag::A)) != 0)
			append_list(result, "A", delim);
		if ((flags.raw_value() & std::to_underlying(MyFlag::B)) != 0)
			append_list(result, "B", delim);
		if ((flags.raw_value() & std::to_underlying(MyFlag::C)) != 0)
			append_list(result, "C", delim);
		if ((flags.raw_value() & std::to_underlying(MyFlag::D)) != 0)
			append_list(result, "D", delim);
		result.append("}");
		return result;
	}
};

TEST_CASE("Flags", "[u][engine][utils][core]") {
	using enum MyFlag;

	SECTION("type properties") {
		CHECK(std::regular<MyFlags>);

		SECTION("default constructor") {
			STATIC_REQUIRE(std::is_default_constructible_v<MyFlags>);
			STATIC_CHECK_FALSE(std::is_trivially_default_constructible_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_default_constructible_v<MyFlags>);
		}
		SECTION("copy constructor") {
			STATIC_REQUIRE(std::is_copy_constructible_v<MyFlags>);
			STATIC_CHECK(std::is_trivially_copy_constructible_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_copy_constructible_v<MyFlags>);
		}
		SECTION("copy assignment") {
			STATIC_REQUIRE(std::is_copy_assignable_v<MyFlags>);
			STATIC_CHECK(std::is_trivially_copy_assignable_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_copy_assignable_v<MyFlags>);
		}
		SECTION("move constructor") {
			STATIC_REQUIRE(std::is_move_constructible_v<MyFlags>);
			STATIC_CHECK(std::is_trivially_move_constructible_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_move_constructible_v<MyFlags>);
		}
		SECTION("move assignment") {
			STATIC_REQUIRE(std::is_move_assignable_v<MyFlags>);
			STATIC_CHECK(std::is_trivially_move_assignable_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_move_assignable_v<MyFlags>);
		}
		SECTION("destructor") {
			STATIC_REQUIRE(std::is_destructible_v<MyFlags>);
			STATIC_CHECK(std::is_trivially_destructible_v<MyFlags>);
			STATIC_CHECK(std::is_nothrow_destructible_v<MyFlags>);
		}
	}

	SECTION("default construct") {
		CHECK(MyFlags{}.raw_value() == 0);
	}
	SECTION("construct from a single flag") {
		constexpr MyFlags f {C};

		CHECK_FALSE(f.test(A));
		CHECK_FALSE(f.test(B));
		CHECK(f.test(C));
		CHECK_FALSE(f.test(D));
	}
	SECTION("construct from a list of flags") {
		constexpr MyFlags f {fr::from_list, {B, C}};

		CHECK_FALSE(f.test(A));
		CHECK(f.test(B));
		CHECK(f.test(C));
		CHECK_FALSE(f.test(D));
	}
	SECTION("construct from an array of flags") {
		constexpr MyFlags f {{B, C}};

		CHECK_FALSE(f.test(A));
		CHECK(f.test(B));
		CHECK(f.test(C));
		CHECK_FALSE(f.test(D));
	}
	SECTION("construct from a range of flags") {
		constexpr MyFlags f {fr::from_range, std::span<const MyFlag>({B, C})};

		CHECK_FALSE(f.test(A));
		CHECK(f.test(B));
		CHECK(f.test(C));
		CHECK_FALSE(f.test(D));
	}
	SECTION("construct, set and test") {
		MyFlags f;

		CHECK_FALSE(f.test(A));
		CHECK_FALSE(f.test(B));
		CHECK_FALSE(f.test(C));
		CHECK_FALSE(f.test(D));

		SECTION("set single flag") {
			f.set(B);
			CHECK_FALSE(f.test(A));
			CHECK(f.test(B));
			CHECK_FALSE(f.test(C));
			CHECK_FALSE(f.test(D));

			f.set(D);
			CHECK_FALSE(f.test(A));
			CHECK(f.test(B));
			CHECK_FALSE(f.test(C));
			CHECK(f.test(D));
		}
		SECTION("set multiple flags") {
			f.set({fr::from_list, {B, D}});
			CHECK_FALSE(f.test(A));
			CHECK(f.test(B));
			CHECK_FALSE(f.test(C));
			CHECK(f.test(D));
		}
	}
	SECTION("test_all_of(..)") {
		constexpr MyFlags f {{A, D}};

		CHECK(f.test_all_of({{A, D}}));
		CHECK(f.test_all_of({D}));
		CHECK_FALSE(f.test_all_of({{A, C, D}}));
	}
	SECTION("test_none_of(..)") {
		constexpr MyFlags f {{A, D}};

		CHECK(f.test_none_of({{B, C}}));
		CHECK(f.test_none_of({B}));
		CHECK_FALSE(f.test_none_of({{C, D}}));
	}
	SECTION("test_any_of(..)") {
		constexpr MyFlags f {{A, D}};

		CHECK(f.test_any_of({{A, D}}));
		CHECK(f.test_any_of({A}));
		CHECK(f.test_any_of({{A, B}}));
		CHECK_FALSE(f.test_any_of({{B, C}}));
		CHECK_FALSE(f.test_any_of({C}));
	}
	SECTION("set(..) specific value") {
		SECTION("set true") {
			constexpr MyFlags f {{A, D}};
			CHECK(MyFlags{f}.set(B, true) == MyFlags{{A, B, D}});
			CHECK(MyFlags{f}.set(A, true) == f);

			CHECK(MyFlags{f}.set(B, true) == MyFlags{{A, B, D}});
			CHECK(MyFlags{f}.set(A, true) == f);
			CHECK(MyFlags{f}.set({{B, C}}, true) == MyFlags{{A, B, C, D}});
			CHECK(MyFlags{f}.set({{A, C}}, true) == MyFlags{{A, C, D}});
		}
		SECTION("set false") {
			constexpr MyFlags f{{A, C, D}};

			CHECK(MyFlags{f}.set(C, false) == MyFlags{{A, D}});
			CHECK(MyFlags{f}.set(B, false) == f);

			CHECK(MyFlags{f}.set(MyFlags{C}, false) == MyFlags{{A, D}});
			CHECK(MyFlags{f}.set({{A, D}}, false) == MyFlags{C});
			CHECK(MyFlags{f}.set({{B, D}}, false) == MyFlags{{A, C}});
		}
	}
	SECTION("reset(..)") {
		constexpr MyFlags f {{A, C, D}};

		CHECK(MyFlags{f}.reset(C) == MyFlags{{A, D}});
		CHECK(MyFlags{f}.reset(B) == f);

		CHECK(MyFlags{f}.reset(MyFlags{C}) == MyFlags{{A, D}});
		CHECK(MyFlags{f}.reset({{A, D}}) == MyFlags{C});
		CHECK(MyFlags{f}.reset({{B, D}}) == MyFlags{{A, C}});
	}
	SECTION("flip(..)") {
		constexpr MyFlags f {{A, D}};

		CHECK(MyFlags{f}.flip(C) == MyFlags{{A, C, D}});
		CHECK(MyFlags{f}.flip(A) == MyFlags{D});
		CHECK(MyFlags{f}.flip({{B, C}}) == MyFlags{{A, B, C, D}});
		CHECK(MyFlags{f}.flip({{A, B}}) == MyFlags{{B, D}});
	}
	SECTION("clear()") {
		CHECK(MyFlags{{A, B}}.clear() == MyFlags{});
		CHECK(MyFlags{}.clear() == MyFlags{});
	}
	SECTION("bool conversion") {
		CHECK(static_cast<bool>(MyFlags{A}));
		CHECK_FALSE(static_cast<bool>(MyFlags{}));
		CHECK_FALSE(!MyFlags{A});
		CHECK(!MyFlags{});
	}
	SECTION("empty()") {
		CHECK(MyFlags{}.empty());
		CHECK_FALSE(MyFlags{{A, B}}.empty());
		CHECK_FALSE(MyFlags{B}.empty());
	}
	SECTION("count()") {
		CHECK(MyFlags{}.count() == 0);
		CHECK(MyFlags{A}.count() == 1);
		CHECK(MyFlags{B}.count() == 1);
		CHECK(MyFlags{{A, D}}.count() == 2);
		CHECK(MyFlags{{B, C}}.count() == 2);
		CHECK(MyFlags{{A, C, D}}.count() == 3);
		CHECK(MyFlags{{A, B, C, D}}.count() == 4);
	}
	SECTION("comparison operators") {
		CHECK(MyFlags{A} == MyFlags{A});
		CHECK(MyFlags{{B, C}} == MyFlags{{B, C}});
		CHECK_FALSE(MyFlags{A} == MyFlags{B});
		CHECK_FALSE(MyFlags{{B, C}} == MyFlags{{B, D}});
		CHECK_FALSE(MyFlags{{B, C, D}} == MyFlags{{B, D}});

		CHECK_FALSE(MyFlags{A} != MyFlags{A});
		CHECK_FALSE(MyFlags{{B, C}} != MyFlags{{B, C}});
		CHECK(MyFlags{A} != MyFlags{B});
		CHECK(MyFlags{{B, C}} != MyFlags{{B, D}});
		CHECK(MyFlags{{B, C, D}} != MyFlags{{B, D}});
	}
	SECTION("bitwise operators") {
		constexpr MyFlags f1 {{A, D}};
		constexpr MyFlags f2 {{B, D}};

		CHECK((f1 | f2) == MyFlags{{A, B, D}});
		CHECK((f1 & f2) == MyFlags{D});
		CHECK((f1 ^ f2) == MyFlags{{A, B}});

		CHECK((f1 | B) == MyFlags{{A, B, D}});
		CHECK((f1 & D) == MyFlags{D});
		CHECK((f1 & B) == MyFlags{});
		CHECK((f1 ^ D) == MyFlags{A});
		CHECK((f1 ^ B) == MyFlags{{A, B, D}});

		CHECK((MyFlags{f1} |= f2) == MyFlags{{A, B, D}});
		CHECK((MyFlags{f1} &= f2) == MyFlags{D});
		CHECK((MyFlags{f1} ^= f2) == MyFlags{{A, B}});
		CHECK((MyFlags{f1} |= B) == MyFlags{{A, B, D}});

		CHECK((MyFlags{f1} &= D) == MyFlags{D});
		CHECK((MyFlags{f1} &= B) == MyFlags{});
		CHECK((MyFlags{f1} ^= D) == MyFlags{A});
		CHECK((MyFlags{f1} ^= B) == MyFlags{{A, B, D}});
	}
}
