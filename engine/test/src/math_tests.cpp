#include "fractal_box/core/angle.hpp"
#include "fractal_box/math/math.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "test_common/test_helpers.hpp"

TEST_CASE("TDeg", "[u][engine][core][math]") {
	const auto do_test = []<class T> {
		SECTION("type properties") {
			CHECK(std::is_default_constructible_v<fr::Deg<T>>);
			CHECK(std::is_nothrow_default_constructible_v<fr::Deg<T>>);
			CHECK(std::is_trivially_copy_constructible_v<fr::Deg<T>>);
			CHECK(std::is_trivially_move_constructible_v<fr::Deg<T>>);
			CHECK(std::is_trivially_copy_assignable_v<fr::Deg<T>>);
			CHECK(std::is_trivially_move_assignable_v<fr::Deg<T>>);
			CHECK(std::is_trivially_destructible_v<fr::Deg<T>>);
		}
		SECTION("arithmetic operators") {
			CHECK(fr::Deg<T>{T{2.}} + fr::Deg<T>{T{3.}} == fr::Deg<T>{T{5.}});
			CHECK(fr::Deg<T>{T{2.}} - fr::Deg<T>{T{3.}} == fr::Deg<T>{T{-1.}});

			CHECK(fr::Deg<T>{T{2.}} * T{3.} == fr::Deg<T>{T{6.}});
			CHECK(T{2.} * fr::Deg<T>{T{3.}} == fr::Deg<T>{T{6.}});

			CHECK(fr::Deg<T>{T{2.}} / T{4.} == fr::Deg<T>{T{0.5}});

			CHECK((fr::Deg<T>{T{2.}} += fr::Deg<T>{T{3.}}).value() == T{5.});
			CHECK((fr::Deg<T>{T{2.}} -= fr::Deg<T>{T{3.}}).value() == T{-1.});
			CHECK((fr::Deg<T>{T{2.}} *= T{3.}).value() == T{6.});
			CHECK((fr::Deg<T>{T{2.}} /= T{4.}).value() == T{0.5});
		}
		SECTION("comparison") {
			CHECK(fr::Deg<T>{T{2.}} == fr::Deg<T>{T{2.}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} == fr::Deg<T>{T{3.}});

			CHECK(fr::Deg<T>{T{2.}} != fr::Deg<T>{T{2.5}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} != fr::Deg<T>{T{2.}});

			CHECK(fr::Deg<T>{T{2.}} > fr::Deg<T>{T{1.5}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} > fr::Deg<T>{T{2.}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} > fr::Deg<T>{T{2.}});

			CHECK(fr::Deg<T>{T{2.}} >= fr::Deg<T>{T{1.5}});
			CHECK(fr::Deg<T>{T{2.}} >= fr::Deg<T>{T{2.}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} >= fr::Deg<T>{T{3.}});

			CHECK(fr::Deg<T>{T{2.}} < fr::Deg<T>{T{2.5}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} < fr::Deg<T>{T{1.5}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} < fr::Deg<T>{T{2.}});

			CHECK(fr::Deg<T>{T{2.}} <= fr::Deg<T>{T{2.5}});
			CHECK(fr::Deg<T>{T{2.}} <= fr::Deg<T>{T{2.}});
			CHECK_FALSE(fr::Deg<T>{T{2.}} <= fr::Deg<T>{T{1.5}});
		}
		SECTION("construct from degrees") {
			CHECK_THAT(fr::Deg<T>{fr::Rad<T>{2.}}.value(),
				Catch::Matchers::WithinULP(glm::degrees(T{2.}), 1));
		}
	};

	frt::typed_section<float>(do_test);
	frt::typed_section<double>(do_test);
}

TEST_CASE("TRad", "[u][engine][core][math]") {
	const auto do_test = []<class T> {
		SECTION("type properties") {
			CHECK(std::is_default_constructible_v<fr::Rad<T>>);
			CHECK(std::is_nothrow_default_constructible_v<fr::Rad<T>>);
			CHECK(std::is_trivially_copy_constructible_v<fr::Rad<T>>);
			CHECK(std::is_trivially_move_constructible_v<fr::Rad<T>>);
			CHECK(std::is_trivially_copy_assignable_v<fr::Rad<T>>);
			CHECK(std::is_trivially_move_assignable_v<fr::Rad<T>>);
			CHECK(std::is_trivially_destructible_v<fr::Rad<T>>);
		}
		SECTION("arithmetic operators") {
			CHECK(fr::Rad<T>{T{2.}} + fr::Rad<T>{T{3.}} == fr::Rad<T>{T{5.}});
			CHECK(fr::Rad<T>{T{2.}} - fr::Rad<T>{T{3.}} == fr::Rad<T>{T{-1.}});

			CHECK(fr::Rad<T>{T{2.}} * T{3.} == fr::Rad<T>{T{6.}});
			CHECK(T{2.} * fr::Rad<T>{T{3.}} == fr::Rad<T>{T{6.}});

			CHECK(fr::Rad<T>{T{2.}} / T{4.} == fr::Rad<T>{T{0.5}});

			CHECK((fr::Rad<T>{T{2.}} += fr::Rad<T>{T{3.}}).value() == T{5.});
			CHECK((fr::Rad<T>{T{2.}} -= fr::Rad<T>{T{3.}}).value() == T{-1.});
			CHECK((fr::Rad<T>{T{2.}} *= T{3.}).value() == T{6.});
			CHECK((fr::Rad<T>{T{2.}} /= T{4.}).value() == T{0.5});
		}
		SECTION("comparison") {
			CHECK(fr::Rad<T>{T{2.}} == fr::Rad<T>{T{2.}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} == fr::Rad<T>{T{3.}});

			CHECK(fr::Rad<T>{T{2.}} != fr::Rad<T>{T{2.5}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} != fr::Rad<T>{T{2.}});

			CHECK(fr::Rad<T>{T{2.}} > fr::Rad<T>{T{1.5}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} > fr::Rad<T>{T{2.}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} > fr::Rad<T>{T{2.}});

			CHECK(fr::Rad<T>{T{2.}} >= fr::Rad<T>{T{1.5}});
			CHECK(fr::Rad<T>{T{2.}} >= fr::Rad<T>{T{2.}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} >= fr::Rad<T>{T{3.}});

			CHECK(fr::Rad<T>{T{2.}} < fr::Rad<T>{T{2.5}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} < fr::Rad<T>{T{1.5}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} < fr::Rad<T>{T{2.}});

			CHECK(fr::Rad<T>{T{2.}} <= fr::Rad<T>{T{2.5}});
			CHECK(fr::Rad<T>{T{2.}} <= fr::Rad<T>{T{2.}});
			CHECK_FALSE(fr::Rad<T>{T{2.}} <= fr::Rad<T>{T{1.5}});
		}
		SECTION("construct from degrees") {
			CHECK_THAT(fr::Rad<T>{fr::Deg<T>{2.}}.value(),
				Catch::Matchers::WithinULP(glm::radians(T{2.}), 1));
		}
	};

	frt::typed_section<float>(do_test);
	frt::typed_section<double>(do_test);
}

TEST_CASE("almost_equal", "[u][engine][core][math]") {
	SECTION("equal numbers are almost equal") {
		CHECK(fr::almost_equal(0., 0., 1));
		CHECK(fr::almost_equal(-1.f, -1.f, 1));
	}

	SECTION("explicitly specified template parameter") {
		CHECK(fr::almost_equal<double>(6.5, 6.5, 2));
		CHECK(fr::almost_equal<float>(6.5f, 6.5f, 2));
	}

	SECTION("different numbers are not almost equal") {
		CHECK(!fr::almost_equal(-6.5f, -7.5f, 2));
		CHECK(!fr::almost_equal(6.5f, -6.5f, 2));
	}

	SECTION("small margin") {
		CHECK(fr::almost_equal(6.5f, std::nextafterf(6.5f, 1.f), 2));
		CHECK(!fr::almost_equal(4.59803f, 4.59804f, 1));
	}

	SECTION("large margin") {
		CHECK(fr::almost_equal(4.59803f, 4.59804f, 50));
	}
}

// TODO: use Approx
TEST_CASE("midpoint", "[u][engine][core][math]") {
	SECTION("float") {
		CHECK(fr::midpoint(1.f, 1.f) == 1.f);
		CHECK(fr::almost_equal(fr::midpoint(100.f, 200.f), 150.f, 1));
		CHECK(fr::almost_equal(fr::midpoint(200.f, 100.f), 150.f, 1));
		CHECK(fr::almost_equal(fr::midpoint(-100.f, 200.f), 50.f, 1));
		CHECK(fr::almost_equal(fr::midpoint(-100.f, -200.f), -150.f, 1));
		CHECK(fr::almost_equal(fr::midpoint(-200.f, -100.f), -150.f, 1));
	}

	SECTION("double") {
		CHECK(fr::almost_equal(fr::midpoint(100., 200.), 150., 1));
		CHECK(fr::almost_equal(fr::midpoint(200., 100.), 150., 1));
		CHECK(fr::almost_equal(fr::midpoint(-100., 200.), 50., 1));
		CHECK(fr::almost_equal(fr::midpoint(-100., -200.), -150., 1));
		CHECK(fr::almost_equal(fr::midpoint(-200., -100.), -150., 1));
	}

	SECTION("glm::vec2") {
		CHECK(fr::midpoint(glm::vec2{1.f, 1.f}, glm::vec2{1.f, 1.f}) == glm::vec2{1.f, 1.f});
		{
			const auto m = fr::midpoint(glm::vec2{-3.f, 10.f}, glm::vec2{-7.f, 56.f});
			CHECK(fr::almost_equal(m.x, -5.f, 1));
			CHECK(fr::almost_equal(m.y, 33.f, 1));
		}
	}
}
