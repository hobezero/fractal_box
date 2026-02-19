#include "fractal_box/core/logging.hpp"

#include "fractal_box/core/preprocessor.hpp"

namespace fr {

void vlog_message(
	std::FILE* out,
	LogLevel log_level,
	std::chrono::system_clock::time_point when,
	std::source_location location,
	fmt::string_view format,
	fmt::format_args args
) {
	FR_UNUSED(location);
	auto buffer = fmt::memory_buffer{};
	fmt::format_to(std::back_inserter(buffer), "[{} {:%T}] ", log_level_prefix_long(log_level),
		when.time_since_epoch());
	fmt::vformat_to(std::back_inserter(buffer), format, args);
	buffer.append(std::string_view{"\n"});

	fmt::detail::print(out, {buffer.data(), buffer.size()});
	std::fflush(out);
}

} // namespace fr
