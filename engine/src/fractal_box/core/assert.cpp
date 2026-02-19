#include "fractal_box/core/assert.hpp"

#include <cstdio>
#include <cstdlib>

#include <fmt/format.h>

#include "fractal_box/core/platform.hpp"

/// Adopted from https://github.com/nemequ/portable-snippets/blob/master/debug-trap/debug-trap.h
#if defined(__has_builtin) && !defined(__ibmxl__)
#	if __has_builtin(__builtin_debugtrap)
#		define FR_DEBUG_TRAP() __builtin_debugtrap()
#	elif __has_builtin(__debugbreak)
#		define FR_DEBUG_TRAP() __debugbreak()
#	endif
#endif
#if !defined(FR_DEBUG_TRAP)
#	if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#		define FR_DEBUG_TRAP() __debugbreak()
#	elif defined(__ARMCC_VERSION)
#		define FR_DEBUG_TRAP() __breakpoint(42)
#	elif defined(__ibmxl__) || defined(__xlC__)
#		include <builtins.h>
#		define FR_DEBUG_TRAP() __trap(42)
#	elif defined(__DMC__) && defined(_M_IX86)
#		define FR_DEBUG_TRAP() __asm int 3h;
#	elif defined(__i386__) || defined(__x86_64__)
#		define FR_DEBUG_TRAP() __asm__ __volatile__("int3")
#	elif defined(__thumb__)
#		define FR_DEBUG_TRAP() __asm__ __volatile__(".inst 0xde01");
#	elif defined(__aarch64__)
#		define FR_DEBUG_TRAP() __asm__ __volatile__(".inst 0xd4200000");
#	elif defined(__arm__)
#		define FR_DEBUG_TRAP() __asm__ __volatile__(".inst 0xe7f001f0");
#	elif defined (__alpha__) && !defined(__osf__)
#		define FR_DEBUG_TRAP() __asm__ __volatile__("bpt");
#	elif defined(_54_)
#		define FR_DEBUG_TRAP() __asm__ __volatile__("ESTOP");
#	elif defined(_55_)
#		define FR_DEBUG_TRAP() \
			__asm__ __volatile__(";\n .if (.MNEMONIC)\n ESTOP_1\n .else\n ESTOP_1()\n .endif\n NOP");
#	elif defined(_64P_)
#		define FR_DEBUG_TRAP() __asm__ __volatile__("SWBP 0");
#	elif defined(_6x_)
#		define FR_DEBUG_TRAP() __asm__ __volatile__("NOP\n .word 0x10000000");
#	elif defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 0) && defined(__GNUC__)
#		define FR_DEBUG_TRAP() __builtin_trap()
#	elif FR_OS_UNIX
#		include <signal.h>
#		if defined(SIGTRAP)
#			define FR_DEBUG_TRAP() raise(SIGTRAP)
#		else
#			define FR_DEBUG_TRAP() raise(SIGABRT)
#		endif
#	else
#		define FR_DEBUG_TRAP() std::abort()
#	endif
#endif

namespace fr {

using namespace std::string_view_literals;

[[noreturn]]
void debug_abort() noexcept {
	FR_DEBUG_TRAP();
	std::abort();
}

static
void print_termination_msg(std::FILE* out, const TerminationRequest& ctx) {
	if (!ctx.expression.empty())
		fmt::print(out, " Condition '{}' failed", ctx.expression);

	if (!ctx.message.empty())
		fmt::print(out, "{} \"{}\"", ctx.expression.empty() ? std::string_view{} : ":"sv,
			ctx.message);

	const auto file_name = std::string_view(ctx.location.file_name());
	fmt::print(out, " {} '{}", ctx.expression.empty() && ctx.message.empty()
		? "At"sv : "at"sv, file_name.empty() ? "(unknown)"sv : file_name);

	if (ctx.location.line() != 0)
		fmt::print(out, ":{}", ctx.location.line());

	if (ctx.location.column() != 0)
		fmt::print(out, ":{}'", ctx.location.column());

	const auto function_name = std::string_view(ctx.location.function_name());
	fmt::print(out, " in function '{}'\n", function_name.empty() ? "(unknown)"sv : function_name);

	std::fflush(out);
}

[[noreturn]]
void default_on_panic(const TerminationRequest& ctx) noexcept {
	auto* out = stderr;

	fmt::print(out, "PANIC!");
	print_termination_msg(out, ctx);

	// Manually inline the `debug_abort()` call to shorten the backtrace produced by debugger.
	// https://nullprogram.com/blog/2022/06/26/
	FR_DEBUG_TRAP();
	std::abort();
}

static constinit auto panic_handler = TerminationRequestHandler{&default_on_panic};

auto get_panic_handler() noexcept -> TerminationRequestHandler {
	return panic_handler;
}

void set_panic_handler(TerminationRequestHandler handler) noexcept {
	panic_handler = handler;
}

void run_panic_handler(const TerminationRequest& ctx) noexcept {
	get_panic_handler()(ctx);
	FR_UNREACHABLE_UNCHECKED();
}

[[noreturn]]
void default_on_assert_violation(const TerminationRequest& ctx) noexcept {
	auto* out = stderr;

	fmt::print(out, "ASSERT!");
	print_termination_msg(out, ctx);

	FR_DEBUG_TRAP();
	std::abort();
}

static constinit TerminationRequestHandler assert_violation_handler = &default_on_assert_violation;

auto get_assert_violation_handler() noexcept -> TerminationRequestHandler {
	return assert_violation_handler;
}

void set_assert_violation_handler(TerminationRequestHandler handler) noexcept {
	assert_violation_handler = handler;
}

void run_assert_violation_handler(const TerminationRequest& ctx) noexcept {
	get_assert_violation_handler()(ctx);
	// Silence false positive warning about return from a `[[noreturn]]` function
	FR_UNREACHABLE_UNCHECKED();
}

} // namespace fr
