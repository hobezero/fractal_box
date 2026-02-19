#include "fractal_box/core/assert_levels.hpp"

#undef FR_OVERRIDE_ASSERT_LEVEL
#define FR_OVERRIDE_ASSERT_LEVEL FR_ASSERT_LEVEL_NONE
#include "fractal_box/core/assert.hpp"

#include <csetjmp>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/scope.hpp"

// NOTE: Catch2 can't run test sections in parallel, so reading static variables is safe
// NOTE: `longjmp` is the only way to safely exit from an assert handler without invoking UB because
// `run_assert_violation_handler` is marked as both `[[noreturn]]` and `noexcept`

static std::jmp_buf jump_point;

TEST_CASE("FR_ASSERT.Disabled", "[u][engine][core][assert]") {
	FR_DEFER [h = fr::get_assert_violation_handler()] { fr::set_assert_violation_handler(h); };
	fr::set_assert_violation_handler(+[](const fr::TerminationRequest&) noexcept {
		std::longjmp(jump_point, 0);
	});

	int var = 0;
	SECTION("FR_ASSERT_FAST") {
		REQUIRE(!FR_ASSERT_FAST_ENABLED);
		SECTION("FR_ASSERT_FAST") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST((var = 1) == 1);
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST((var = 2) == 1);
				CHECK(var == 0);
			}
		}
		SECTION("FR_ASSERT_FAST_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST_MSG((var = 1) == 1, "test");
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_FAST_MSG((var = 2) == 1, "test");
				CHECK(var == 0);
			}
		}
	}
	SECTION("FR_ASSERT (default)") {
		REQUIRE(!FR_ASSERT_ENABLED);
		SECTION("FR_ASSERT") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT((var = 1) == 1);
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT((var = 2) == 1);
				CHECK(var == 0);
			}
		}
		SECTION("FR_ASSERT_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_MSG((var = 1) == 1, "test");
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_MSG((var = 2) == 1, "test");
				CHECK(var == 0);
			}
		}
	}
	SECTION("FR_ASSERT_AUDIT") {
		REQUIRE(!FR_ASSERT_AUDIT_ENABLED);
		SECTION("FR_ASSERT_AUDIT") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT((var = 1) == 1);
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT((var = 2) == 1);
				CHECK(var == 0);
			}
		}
		SECTION("FR_ASSERT_AUDIT_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT_MSG((var = 1) == 1, "test");
				CHECK(var == 0);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_ASSERT_AUDIT_MSG((var = 2) == 1, "test");
				CHECK(var == 0);
			}
		}
	}
}

TEST_CASE("FR_VERIFY.Disabled", "[u][engine][core][assert]") {
	FR_DEFER [h = fr::get_assert_violation_handler()] { fr::set_assert_violation_handler(h); };
	fr::set_assert_violation_handler(+[](const fr::TerminationRequest&) noexcept {
		FAIL();
		std::longjmp(jump_point, 0);
	});

	int var = 0;
	SECTION("FR_VERIFY_FAST") {
		REQUIRE(!FR_VERIFY_FAST_ENABLED);
		SECTION("FR_VERIFY_FAST") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST((var = 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST((var = 2) == 1);
				CHECK(var == 2);
			}
		}
		SECTION("FR_VERIFY_FAST_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST_MSG((var = 1) == 1, "test");
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_FAST_MSG((var = 2) == 1, "test");
				CHECK(var == 2);
			}
		}
	}
	SECTION("FR_VERIFY (default)") {
		REQUIRE(!FR_VERIFY_ENABLED);
		SECTION("FR_VERIFY") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY((var = 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY((var = 2) == 1);
				CHECK(var == 2);
			}
		}
		SECTION("FR_VERIFY_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_MSG((var = 1) == 1, "test");
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_MSG((var = 2) == 1, "test");
				CHECK(var == 2);
			}
		}
	}
	SECTION("FR_VERIFY_AUDIT") {
		REQUIRE(!FR_VERIFY_AUDIT_ENABLED);
		SECTION("FR_VERIFY_AUDIT") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT((var = 1) == 1);
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT((var = 2) == 1);
				CHECK(var == 2);
			}
		}
		SECTION("FR_VERIFY_MSG") {
			SECTION("condition followed") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT_MSG((var = 1) == 1, "test");
				CHECK(var == 1);
			}
			SECTION("condition violated") {
				if (setjmp(jump_point))
					FAIL();
				else
					FR_VERIFY_AUDIT_MSG((var = 2) == 1, "test");
				CHECK(var == 2);
			}
		}
	}
}
