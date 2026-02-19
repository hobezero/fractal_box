#ifndef FRACTAL_BOX_CORE_ASSERT_FMT_HPP
#define FRACTAL_BOX_CORE_ASSERT_FMT_HPP

#include <fmt/format.h>

#include "fractal_box/core/assert.hpp"

// Basic macros
// ------------

#define FR_PANIC_FMT(fmt_str, ...) FR_PANIC_MSG(::fmt::format(fmt_str, __VA_ARGS__))

#define FR_PANIC_CHECK_FMT(expr, fmt_str, ...) \
	FR_PANIC_CHECK_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#define FR_ASSERT_CHECK_FMT(expr, fmt_str, ...) \
	FR_ASSERT_CHECK_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

// ASSERT
// ------

#define FR_ASSERT_FAST_FMT(expr, fmt_str, ...) \
	FR_ASSERT_FAST_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#define FR_ASSERT_FMT(expr, fmt_str, ...) \
	FR_ASSERT_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#define FR_ASSERT_AUDIT_FMT(expr, fmt_str, ...) \
	FR_ASSERT_AUDIT_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

// VERIFY
// ------

#define FR_VERIFY_FAST_FMT(expr, fmt_str, ...) \
	FR_VERIFY_FAST_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#define FR_VERIFY_FMT(expr, fmt_str, ...) \
	FR_VERIFY_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#define FR_VERIFY_AUDIT_FMT(expr, fmt_str, ...) \
	FR_VERIFY_AUDIT_MSG(expr, ::fmt::format(fmt_str, __VA_ARGS__))

#endif
