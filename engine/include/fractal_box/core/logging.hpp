#ifndef FRACTAL_BOX_CORE_LOGGING_HPP
#define FRACTAL_BOX_CORE_LOGGING_HPP

#include <cstdio>

#include <chrono>
#include <source_location>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <hedley.h>

#include "fractal_box/core/log_levels.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr {

enum class LogLevel: int {
	None = FR_LOG_LEVEL_NONE,
	Fatal = FR_LOG_LEVEL_FATAL,
	Error = FR_LOG_LEVEL_ERROR,
	Warn = FR_LOG_LEVEL_WARN,
	Info = FR_LOG_LEVEL_INFO,
	Debug = FR_LOG_LEVEL_DEBUG,
	Trace = FR_LOG_LEVEL_TRACE,
};

inline constexpr
auto log_level_prefix_long(LogLevel log_level) noexcept -> std::string_view {
	using enum LogLevel;
	switch (log_level) {
		case None: return "NONE "; // I don't think we shoud ever hit this line
		case Fatal: return "FATAL";
		case Error: return "ERROR";
		case Warn: return "WARN ";
		case Trace: return "TRACE";
		case Info: return "INFO ";
		case Debug: return "DEBUG";
	}
	return "UNKNOWN";
}

FR_NOINLINE
void vlog_message(
	std::FILE* out,
	LogLevel log_level,
	std::chrono::system_clock::time_point when,
	std::source_location location,
	fmt::string_view format,
	fmt::format_args args
);

template<class... Args>
FR_FORCE_INLINE constexpr
void log_message_now(
	std::FILE* out,
	LogLevel log_level,
	std::source_location location,
	fmt::format_string<Args...> format,
	Args&&... args
) {
	if !consteval {
		const auto when = std::chrono::system_clock::now();
		vlog_message(out, log_level, when, location, format, fmt::make_format_args(args...));
	}
}

} // namespace fr

#define FR_DETAIL_LOG_NOOP(log_level, format_str, ...) \
	static_cast<void>(sizeof(::fmt::format(format_str, __VA_ARGS__)))

#define FR_LOG_ALWAYS(log_level, format_str, ...) \
	::fr::log_message_now(stdout, log_level, ::std::source_location::current(), \
		format_str, __VA_ARGS__)

#if (defined(FR_OVERRIDE_LOG_LEVEL) && FR_OVERRIDE_LOG_LEVEL >= FR_LOG_LEVEL_NONE)
	// Clamp log level into the supported range
#	if FR_OVERRIDE_LOG_LEVEL > FR_LOG_LEVEL_MAX
		HEDLEY_WARNING("Unsupported FR_OVERRIDE_LOG_LEVEL. Using the value of " \
			"FR_OVERRIDE_LOG_LEVEL instead")
#		define FR_ASSERT_LEVEL FR_LOG_LEVEL_MAX
#	else
#		define FR_LOG_LEVEL FR_OVERRIDE_LOG_LEVEL
#	endif
#else
#	if (defined(FR_OVERRIDE_LOG_LEVEL) /* NOLINT */ \
		&& FR_OVERRIDE_LOG_LEVEL != FR_LOG_LEVEL_AUTO \
	)
		HEDLEY_WARNING("Unsupported FR_OVERRIDE_LOG_LEVEL. Using the value of " \
			"FR_LOG_LEVEL_AUTODETECT instead")
#	endif
	// Autodetect log level only if FR_OVERRIDE_LOG_LEVEL is negative or hasn't been
	// explicitly set
#	ifdef NDEBUG
#		define FR_LOG_LEVEL FR_LOG_LEVEL_INFO
#	else
#		define FR_LOG_LEVEL FR_LOG_LEVEL_DEBUG
#	endif
#endif

#undef FR_LOG

#define FR_LOG(log_level, fmt, ...) \
	FR_CONDITIONAL(FR_LOG_LEVEL >= log_level, \
		FR_LOG_ALWAYS, \
		FR_DETAIL_LOG_NOOP \
	)(__VA_ARGS__)

// FATAL
#undef FR_LOG_FATAL
#undef FR_LOG_FATAL_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_FATAL
#	define FR_LOG_FATAL(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Fatal, format, __VA_ARGS__)
#	define FR_LOG_FATAL_ENABLED 1
#else
#	define FR_LOG_FATAL(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Fatal, format, __VA_ARGS__)
#	define FR_LOG_FATAL_ENABLED 0
#endif
#define FR_LOG_FATAL_MSG(msg) FR_LOG_FATAL("{}", msg)

// ERROR
#undef FR_LOG_ERROR
#undef FR_LOG_ERROR_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_ERROR
#	define FR_LOG_ERROR(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Error, format, __VA_ARGS__)
#	define FR_LOG_ERROR_ENABLED 1
#else
#	define FR_LOG_ERROR(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Error, format, __VA_ARGS__)
#	define FR_LOG_ERROR_ENABLED 0
#endif
#define FR_LOG_ERROR_MSG(msg) FR_LOG_ERROR("{}", msg)

// WARN
#undef FR_LOG_WARN
#undef FR_LOG_WARN_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_WARN
#	define FR_LOG_WARN(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Warn, format, __VA_ARGS__)
#	define FR_LOG_WARN_ENABLED 1
#else
#	define FR_LOG_WARN(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Warn, format, __VA_ARGS__)
#	define FR_LOG_WARN_ENABLED 0
#endif
#define FR_LOG_WARN_MSG(msg) FR_LOG_WARN("{}", msg)

// INFO
#undef FR_LOG_INFO
#undef FR_LOG_INFO_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_INFO
#	define FR_LOG_INFO(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Info, format, __VA_ARGS__)
#	define FR_LOG_INFO_ENABLED 1
#else
#	define FR_LOG_INFO(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Info, format, __VA_ARGS__)
#	define FR_LOG_INFO_ENABLED 0
#endif
#define FR_LOG_INFO_MSG(msg) FR_LOG_INFO("{}", msg)

// DEBUG
#undef FR_LOG_DEBUG
#undef FR_LOG_DEBUG_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_DEBUG
#	define FR_LOG_DEBUG(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Debug, format, __VA_ARGS__)
#	define FR_LOG_DEBUG_ENABLED 1
#else
#	define FR_LOG_DEBUG(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Debug, format, __VA_ARGS__)
#	define FR_LOG_DEBUG_ENABLED 0
#endif
#define FR_LOG_DEBUG_MSG(msg) FR_LOG_DEBUG("{}", msg)

// TRACE
#undef FR_LOG_TRACE
#undef FR_LOG_TRACE_ENABLED

#if FR_LOG_LEVEL >= FR_LOG_LEVEL_TRACE
#	define FR_LOG_TRACE(format, ...) FR_LOG_ALWAYS( \
		::fr::LogLevel::Trace, format, __VA_ARGS__)
#	define FR_LOG_TRACE_ENABLED 1
#else
#	define FR_LOG_TRACE(format, ...) FR_DETAIL_LOG_NOOP( \
		::fr::LogLevel::Trace, format, __VA_ARGS__)
#	define FR_LOG_TRACE_ENABLED 0
#endif
#define FR_LOG_TRACE_MSG(msg) FR_LOG_TRACE("{}", msg)

#endif // include guard
