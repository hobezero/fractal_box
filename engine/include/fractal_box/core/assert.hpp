#ifndef FRACTAL_BOX_CORE_ASSERT_HPP
#define FRACTAL_BOX_CORE_ASSERT_HPP

/// @file
/// @brief `assert` on steroids
/// @note Include `assert_levels.hpp` first to get `FR_ASSERT_LEVEL_*` macros
///
/// Inspired by https://www.youtube.com/watch?v=mmyIZzqh5ls
///
/// Configation options:
///   - Macro `FR_OVERRIDE_ASSERT_LEVEL`. Possible integer values:
///       - *<undefined>* or `FR_ASSERT_LEVEL_AUTO` (-1) or any negative value
///         => Automatically set assertion level to 1 if NDEBUG is defined, 2 otherwise
///       - `FR_ASSERT_LEVEL_NONE` (0) => No assertion checks are performed
///       - `FR_ASSERT_LEVEL_FAST` (1) => Only `FAST` assertion checks are enabled
///       - `FR_ASSERT_LEVEL_DEFAULT` (2) => Only `FAST` and default assertion checks
///          are enabled
///       - `FR_ASSERT_LEVEL_AUDIT` (3) or greater => `FAST`, default, and `AUDIT` assertion
///          checks are enabled
///       - `FR_ASSER_LEVEL_MAX` (3) equivalent to `FR_ASSERT_LEVEL_AUDIT`
///   - Function `set_assert_violation_handler` allows user to customize the behavior of
///     violation handler
/// @todo:
///   TODO: `ASSUME` levels
///   TODO: Are implicit conversions to bool allowed? Fix inconsitencies in Debug vs Release builds
///   TODO: Assertion categories to allow a user to enable/disable assertion checks granularly for
///         the specific functionality or a module
///   TODO: Consider implementing `FR_PANIC_IF`, `FR_PANIC_IF_MSG`. Be careful with
///         expression negation

#include <source_location>
#include <string_view>

#include "fractal_box/core/assert_levels.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @brief Cause abnormal program termination in a way that the attached debugger will be able to
/// trap the signal and preserve the call stack
[[noreturn]]
void debug_abort() noexcept;

/// @brief A simple constexpr-unfriendly (note the `[[noreturn]]`) declaration used to abort
/// compilation on constant-evaluated assertion failure
[[noreturn]]
void constexpr_assert_fail();

struct TerminationRequest {
	/// @brief Assertion expression as a string
	std::string_view expression;
	std::string_view message;
	std::source_location location;
};

/// @note `[[noreturn]]` can't be a part of the function type
using TerminationRequestHandler = void (*)(const TerminationRequest& ctx) noexcept;

/// @brief Default panic handler. Logs the error to `stderr` and terminates the program
[[noreturn]]
void default_on_panic(const TerminationRequest& ctx) noexcept;

auto get_panic_handler() noexcept -> TerminationRequestHandler;
void set_panic_handler(TerminationRequestHandler handler) noexcept;

[[noreturn]]
void run_panic_handler(const TerminationRequest& ctx) noexcept;

/// @brief Default assert violation handler. Logs the error to `stderr` and terminates the program
[[noreturn]]
void default_on_assert_violation(const TerminationRequest& ctx) noexcept;

auto get_assert_violation_handler() noexcept -> TerminationRequestHandler;
void set_assert_violation_handler(TerminationRequestHandler handler) noexcept;

[[noreturn]]
void run_assert_violation_handler(const TerminationRequest& ctx) noexcept;

} // namespace fr

// Basic macros
// ------------

/// @brief Abnormally terminate program execution by calling current panic handler
#define FR_PANIC() \
	(::fr::run_panic_handler({{}, {}, ::std::source_location::current()}))

/// @brief Abnormally terminate program execution by calling current panic handler with `msg`
/// given as a human-readable message/reason of termination
#define FR_PANIC_MSG(msg) \
	(::fr::run_panic_handler({{}, msg, ::std::source_location::current()}))


/// @brief If `expr` evaluates to `false`, abnormally terminate program execution by calling current
/// panic handler
#define FR_PANIC_CHECK(expr) \
	(FR_LIKELY(expr) \
		? static_cast<void>(0) \
		: (::fr::run_panic_handler({#expr, {}, ::std::source_location::current()})))

#define FR_PANIC_CHECK_MSG(expr, msg) \
	(FR_LIKELY(expr) \
		? static_cast<void>(0) \
		: (::fr::run_panic_handler({#expr, msg, ::std::source_location::current()})))

/// @brief A helper macro to make sure that `expr` is at least syntactically correct and
/// convertible to `bool` but don't actually evaluate the `expr` (unless in a constexpr context)
#define FR_DETAIL_ASSERT_NOOP(expr) \
	do { \
FR_DIAGNOSTIC_PUSH \
FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF \
		if consteval { \
			if (!(expr)) \
				::fr::constexpr_assert_fail(); \
		} \
FR_DIAGNOSTIC_POP \
	} while (false)

/// @brief A helper macro to make sure that `expr` is at least syntactically correct and
/// convertible to `bool` while `msg` is convertible to `std::string_view` but don't actually
/// evaluate `expr` (unless in a constexpr context)
/// @todo TODO: Consider `__VA_ARGS__`
#define FR_DETAIL_ASSERT_NOOP_MSG(expr, msg) \
	do { \
FR_DIAGNOSTIC_PUSH \
FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF \
		static_cast<void>(sizeof(static_cast<::std::string_view>(msg))); \
		if consteval { \
			if (!(expr)) \
				::fr::constexpr_assert_fail(); \
		} \
FR_DIAGNOSTIC_POP \
	} while (false)

/// @brief A helper macro used as a fallback in `FR_VERIFY_*` that always evaluates `expr` but
/// doesn't perform the check (unless in a constexpr context)
#define FR_DETAIL_ASSERT_EVAL(expr) \
	do { \
FR_DIAGNOSTIC_PUSH \
FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF \
		if consteval { \
			if (!(expr)) \
				::fr::constexpr_assert_fail(); \
		} \
		else { \
			static_cast<void>(expr); \
		} \
FR_DIAGNOSTIC_POP \
	} while (false)

/// @brief A helper macro used as a fallback in `FR_VERIFY_*` that always evaluates `expr` and
/// checks that `msg` is convertible to `std::string_view` but doesn't perform the check
/// (unless in a constexpr context)
#define FR_DETAIL_ASSERT_EVAL_MSG(expr, msg) \
	do { \
FR_DIAGNOSTIC_PUSH \
FR_DIAGNOSTIC_DISABLE_REDUNDANT_CONSTEVAL_IF \
		static_cast<void>(sizeof(static_cast<::std::string_view>(msg))); \
		if consteval { \
			if (!(expr)) \
				::fr::constexpr_assert_fail(); \
		} \
		else { \
			static_cast<void>(expr); \
		} \
FR_DIAGNOSTIC_POP \
	} while (false)

/// @brief If `expr` evaulates to `false`, abnormally terminate program execution by calling current
/// assert violation handler
/// @note Macro is always expanded to a check regardless of the `FR_ASSERT_LEVEL`. It behaves
/// almost like `FR_PANIC_CHECK`, but `get_assert_violation_handler()` gets called
/// @note Direct usage is discouraged unless you are manually checking the assertion level for
/// perfomance reasons, e.g:
/// ```cpp
/// #if FR_ASSERT_AUDIT_ENABLED
/// for (const auto& x : my_complex_container)
/// 	FR_ASSERT_CHECK(x);
/// #endif
/// ```
/// @note The behavior of the assert violation handler can be customized,
/// see `set_assert_violation_handler(..)` for details
/// @note Works in constexpr contexts
#define FR_ASSERT_CHECK(expr) \
	(FR_LIKELY(expr) \
		? static_cast<void>(0) \
		: (::fr::run_assert_violation_handler({#expr, {}, ::std::source_location::current()})))

/// @brief The same as `FR_ASSERT_CHECK`, except an error message can be provided which should
/// be printed in the failure case
#define FR_ASSERT_CHECK_MSG(expr, msg) \
	(FR_LIKELY(expr) \
		? static_cast<void>(0) \
		: (::fr::run_assert_violation_handler({#expr, msg, ::std::source_location::current()})))

// ASSERT_LEVEL detection
// ----------------------

// TODO: Are undef's here and further below really needed to support modifying assertion levels
// in the middle of a translation unit? Second inclusion of "assert.hpp" is protected by an
// include guard anyway
#undef FR_ASSERT_LEVEL

#if (defined(FR_OVERRIDE_ASSERT_LEVEL) \
	&& FR_OVERRIDE_ASSERT_LEVEL >= FR_ASSERT_LEVEL_NONE \
)
	// Clamp assert level into the supported range
#	if FR_OVERRIDE_ASSERT_LEVEL > FR_ASSERT_LEVEL_MAX
		HEDLEY_WARNING("Unsupported FR_OVERRIDE_ASSERT_LEVEL. Using the value of " \
			"FR_ASSERT_LEVEL_MAX instead")
#		define FR_ASSERT_LEVEL FR_ASSERT_LEVEL_MAX
#	else
#		define FR_ASSERT_LEVEL FR_OVERRIDE_ASSERT_LEVEL
#	endif
#else
#	if (defined(FR_OVERRIDE_ASSERT_LEVEL) /* NOLINT */ \
		&& FR_OVERRIDE_ASSERT_LEVEL != FR_ASSERT_LEVEL_AUTO \
	)
		HEDLEY_WARNING("Unsupported FR_OVERRIDE_ASSERT_LEVEL. Using the value of " \
			"FR_ASSERT_LEVEL_AUTO instead")
#	endif
	// Autodetect assertion level only if FR_OVERRIDE_ASSERT_LEVEL is negative or hasn't been
	// explicitly set
#	ifdef NDEBUG
#		define FR_ASSERT_LEVEL FR_ASSERT_LEVEL_FAST
#	else
#		define FR_ASSERT_LEVEL FR_ASSERT_LEVEL_DEFAULT
#	endif
#endif

// ASSERT
// ------

// ASSERT_FAST
// ^^^^^^^^^^^

#undef FR_ASSERT_FAST_ENABLED
#undef FR_ASSERT_FAST
#undef FR_ASSERT_FAST_MSG

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_FAST
#	define FR_ASSERT_FAST_ENABLED 1
#	define FR_ASSERT_FAST(expr) FR_ASSERT_CHECK(expr)
#	define FR_ASSERT_FAST_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_ASSERT_FAST_ENABLED 0
#	define FR_ASSERT_FAST(expr) FR_DETAIL_ASSERT_NOOP(expr)
#	define FR_ASSERT_FAST_MSG(expr, msg) FR_DETAIL_ASSERT_NOOP_MSG(expr, msg)
#endif

// ASSERT (default)
// ^^^^^^^^^^^^^^^^

#undef FR_ASSERT_ENABLED
#undef FR_ASSERT
#undef FR_ASSERT_MSG

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_DEFAULT
#	define FR_ASSERT_ENABLED 1
#	define FR_ASSERT(expr) FR_ASSERT_CHECK(expr)
#	define FR_ASSERT_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_ASSERT_ENABLED 0
#	define FR_ASSERT(expr) FR_DETAIL_ASSERT_NOOP(expr)
#	define FR_ASSERT_MSG(expr, msg) FR_DETAIL_ASSERT_NOOP_MSG(expr, msg)
#endif

// ASSERT_AUDIT
// ^^^^^^^^^^^^

#undef FR_ASSERT_AUDIT_ENABLED
#undef FR_ASSERT_AUDIT
#undef FR_ASSERT_AUDIT_MSG

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_AUDIT
#	define FR_ASSERT_AUDIT_ENABLED 1
#	define FR_ASSERT_AUDIT(expr) FR_ASSERT_CHECK(expr)
#	define FR_ASSERT_AUDIT_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_ASSERT_AUDIT_ENABLED 0
#	define FR_ASSERT_AUDIT(expr) FR_DETAIL_ASSERT_NOOP(expr)
#	define FR_ASSERT_AUDIT_MSG(expr, msg) FR_DETAIL_ASSERT_NOOP_MSG(expr, msg)
#endif

// VERIFY
// ------

// VERIFY_FAST
// ^^^^^^^^^^^

#undef FR_VERIFY_FAST_ENABLED
#undef FR_VERIFY_FAST
#undef FR_VERIFY_FAST_MSG

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_FAST
#	define FR_VERIFY_FAST_ENABLED 1
#	define FR_VERIFY_FAST(expr) FR_ASSERT_CHECK(expr)
#	define FR_VERIFY_FAST_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_VERIFY_FAST_ENABLED 0
#	define FR_VERIFY_FAST(expr) FR_DETAIL_ASSERT_EVAL(expr)
#	define FR_VERIFY_FAST_MSG(expr, msg) FR_DETAIL_ASSERT_EVAL_MSG(expr, msg)
#endif

// VERIFY (default)
// ^^^^^^^^^^^^^^^^

#undef FR_VERIFY
#undef FR_VERIFY_MSG
#undef FR_VERIFY_ENABLED

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_DEFAULT
#	define FR_VERIFY_ENABLED 1
	/// @brief The same as `FR_ASSERT`, except the `expr` expression is always evaluated
#	define FR_VERIFY(expr) FR_ASSERT_CHECK(expr)
#	define FR_VERIFY_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_VERIFY_ENABLED 0
	/// @brief The same as `FR_ASSERT`, except the `expr` expression is always evaluated
#	define FR_VERIFY(expr) FR_DETAIL_ASSERT_EVAL(expr)
#	define FR_VERIFY_MSG(expr, msg) FR_DETAIL_ASSERT_EVAL_MSG(expr, msg)
#endif

// VERIFY_AUDIT
// ^^^^^^^^^^^^

#undef FR_VERIFY_AUDIT
#undef FR_VERIFY_AUDIT_MSG
#undef FR_VERIFY_AUDIT_ENABLED

#if FR_ASSERT_LEVEL >= FR_ASSERT_LEVEL_AUDIT
#	define FR_VERIFY_AUDIT_ENABLED 1
#	define FR_VERIFY_AUDIT(expr) FR_ASSERT_CHECK(expr)
#	define FR_VERIFY_AUDIT_MSG(expr, msg) FR_ASSERT_CHECK_MSG(expr, msg)
#else
#	define FR_VERIFY_AUDIT_ENABLED 0
#	define FR_VERIFY_AUDIT(expr) FR_DETAIL_ASSERT_EVAL(expr)
#	define FR_VERIFY_AUDIT_MSG(expr, msg) FR_DETAIL_ASSERT_EVAL_MSG(expr, msg)
#endif

// TODO: lambda based
// FR_VERIFY_FAST_GET
// FR_VERIFY_GET
// FR_VERIFY_AUDIT_GET

// UNREACHABLE
// -----------

// UNREACHABLE_FAST
// ^^^^^^^^^^^^^^^^

#undef FR_UNREACHABLE_FAST_MSG
#undef FR_UNREACHABLE_FAST

#define FR_UNREACHABLE_FAST_MSG(msg) \
	do { \
		FR_ASSERT_FAST(false, msg); \
		FR_UNREACHABLE_UNCHECKED(); \
	} while (false)

#define FR_UNREACHABLE_FAST() FR_UNREACHABLE_FAST_MSG("Reached unreachable")

// UNREACHABLE (default)
// ^^^^^^^^^^^^^^^^^^^^^

#undef FR_UNREACHABLE_MSG
#undef FR_UNREACHABLE

#define FR_UNREACHABLE_MSG(msg) \
	do { \
		FR_ASSERT_MSG(false, msg); \
		FR_UNREACHABLE_UNCHECKED(); \
	} while (false)

#define FR_UNREACHABLE() FR_UNREACHABLE_MSG("Reached unreachable")

// UNREACHABLE_AUDIT
// ^^^^^^^^^^^^^^^^^

#undef FR_UNREACHABLE_AUDIT_MSG
#undef FR_UNREACHABLE_AUDIT

#define FR_UNREACHABLE_AUDIT_MSG(msg) \
	do { \
		FR_ASSERT_AUDIT_MSG(false, msg); \
		FR_UNREACHABLE_UNCHECKED(); \
	} while (false)

#define FR_UNREACHABLE_AUDIT() FR_UNREACHABLE_AUDIT_MSG("Reached unreachable")

// ASSUME
// ------

// ASSUME_FAST
// ^^^^^^^^^^^

#undef FR_ASSUME_FAST_MSG
#undef FR_ASSUME_FAST

#define FR_ASSUME_FAST_MSG(expr, msg) \
	do { \
		FR_ASSERT_FAST_MSG(expr, msg); \
		FR_ASSUME_UNCHECKED(expr); \
	} while (false)

#define FR_ASSUME_FAST(expr) FR_ASSUME_FAST_MSG(expr, "Assumption failed")

// ASSUME (default)
// ^^^^^^^^^^^^^^^^

#undef FR_ASSUME_MSG
#undef FR_ASSUME

#define FR_ASSUME_MSG(expr, msg) \
	do { \
		FR_ASSERT_MSG(expr, msg); \
		FR_ASSUME_UNCHECKED(expr); \
	} while (false)

#define FR_ASSUME(expr) FR_ASSUME_MSG(expr, "Assumption failed")

// ASSUME_AUDIT
// ^^^^^^^^^^^^

#undef FR_ASSUME_AUDIT_MSG
#undef FR_ASSUME_AUDIT

#define FR_ASSUME_AUDIT_MSG(expr, msg) \
	do { \
		FR_ASSERT_AUDIT_MSG(expr, msg); \
		FR_ASSUME_UNCHECKED(expr); \
	} while (false)

#define FR_ASSUME_AUDIT(expr) FR_ASSUME_AUDIT_MSG(expr, "Assumption failed")

#endif // include guard
