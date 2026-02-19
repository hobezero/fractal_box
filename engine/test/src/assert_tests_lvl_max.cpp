#include "fractal_box/core/assert_levels.hpp"

#undef FR_OVERRIDE_ASSERT_LEVEL
#define FR_OVERRIDE_ASSERT_LEVEL FR_ASSERT_LEVEL_MAX
#include "fractal_box/core/assert.hpp"

#include <csetjmp>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/scope.hpp"

// NOTE: Catch2 can't run test sections in parallel, so reading static variables is safe
// NOTE: `longjmp` is the only way to safely exit from an assert handler without invoking UB because
// `run_assert_violation_handler` is marked as both `[[noreturn]]` and `noexcept`

static std::jmp_buf jump_point;

/// @brief Make sure that our dummy variable won't get optimized out
static FR_NOINLINE
auto set_var(volatile int& var, int value) -> int {
	var = value;
	return var;
}

TEST_CASE("FR_ASSERT.Enabled", "[u][engine][core][assert]") {
	// We need static/global data because assert handler doesn't support user-data
	// static int violation_counter = 0;
	static constexpr std::string_view empty_msg;
	static constexpr std::string_view test_message = "TestMessage";
	static std::string_view expected_msg;

	FR_DEFER [h = fr::get_assert_violation_handler()] { fr::set_assert_violation_handler(h); };
	fr::set_assert_violation_handler(+[](const fr::TerminationRequest& violation) noexcept {
		{
			// We need to `CHECK` here because of potential lifetime issues
			CHECK(violation.message == expected_msg);
			CHECK_FALSE(violation.expression.empty());
			CHECK_FALSE(std::string_view(violation.location.file_name()).empty());
			CHECK_FALSE(std::string_view(violation.location.function_name()).empty());
			CHECK(violation.location.line() != 0);
		}
		std::longjmp(jump_point, 0);
	});

	volatile int var = 0;
	SECTION("ASSERT_FAST") {
		REQUIRE(FR_ASSERT_FAST_ENABLED);
		SECTION("FR_ASSERT_FAST") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT_FAST(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_ASSERT_FAST_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT_FAST_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
	SECTION("ASSERT (default)") {
		REQUIRE(FR_ASSERT_ENABLED);
		SECTION("FR_ASSERT") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_ASSERT_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
	SECTION("ASSERT_AUDIT") {
		REQUIRE(FR_ASSERT_AUDIT_ENABLED);
		SECTION("FR_ASSERT_AUDIT") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT_AUDIT(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_ASSERT_AUDIT_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_ASSERT_AUDIT_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
}

TEST_CASE("FR_VERIFY.Enabled", "[u][engine][core][assert]") {
	// We need static/global data because assert handler doesn't support user-data
	static constexpr std::string_view empty_msg;
	static constexpr std::string_view test_message = "TestMessage";
	static std::string_view expected_msg;

	FR_DEFER [h = fr::get_assert_violation_handler()] { fr::set_assert_violation_handler(h); };
	fr::set_assert_violation_handler(+[](const fr::TerminationRequest& violation) noexcept {
		{
			// We need to `CHECK` here because of potential lifetime issues
			CHECK(violation.message == expected_msg);
			CHECK_FALSE(violation.expression.empty());
			CHECK_FALSE(std::string_view(violation.location.file_name()).empty());
			CHECK_FALSE(std::string_view(violation.location.function_name()).empty());
			CHECK(violation.location.line() != 0);
		}
		std::longjmp(jump_point, 0);
	});

	volatile int var = 0;
	SECTION("VERIFY_FAST") {
		REQUIRE(FR_VERIFY_FAST_ENABLED);
		SECTION("FR_VERIFY_FAST") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY_FAST(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_VERIFY_FAST_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY_FAST_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
	SECTION("VERIFY (default)") {
		REQUIRE(FR_VERIFY_ENABLED);
		SECTION("FR_VERIFY") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_VERIFY_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
	SECTION("VERIFY_AUDIT") {
		REQUIRE(FR_VERIFY_AUDIT_ENABLED);
		SECTION("FR_VERIFY_AUDIT") {
			expected_msg = empty_msg;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT(set_var(var, 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY_AUDIT(set_var(var, 2) == 1);
					FAIL();
				}
			}
		}
		SECTION("FR_VERIFY_AUDIT_MSG") {
			expected_msg = test_message;
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT_MSG(set_var(var, 1) == 1, test_message);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					CHECK(var == 2);
				else {
					FR_VERIFY_AUDIT_MSG(set_var(var, 2) == 1, test_message);
					FAIL();
				}
			}
		}
	}
}
