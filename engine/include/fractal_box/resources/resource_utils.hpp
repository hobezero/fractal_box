#ifndef FRACTAL_BOX_RESOURCES_RESOURCE_UTIL_HPP
#define FRACTAL_BOX_RESOURCES_RESOURCE_UTIL_HPP

#include <span>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <cmrc/cmrc.hpp>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/error_handling/diagnostic.hpp"

namespace fr {

inline auto
try_get_resource_data(
	cmrc::embedded_filesystem fs,
	const std::string& file_name
) -> std::optional<std::span<const char>> {
	// We lose a bit of performance here in debug mode by checking if file exists twice,
	// second check being in `fs.open(..)`. CMRC doesn't have `find(..)` function or equivalent.
	// An alternative solution would be to wrap `fs.open(..)` into a try/catch block,
	// but it's just ugly
	if (!fs.exists(file_name)) {
		return std::nullopt;
	}
	const auto file_data = fs.open(file_name);
	return std::span{file_data.begin(), file_data.end()};
}

inline
auto get_resource_data(
	cmrc::embedded_filesystem fs,
	const std::string& file_name
) -> std::span<const char> {
	auto file_data = try_get_resource_data(fs, file_name);
	FR_PANIC_CHECK_FMT(file_data, "Resource file '{}' not found", file_name);
	return *file_data;
}

class ResourceNotFound: public ErrorBase {
public:
	explicit
	ResourceNotFound(std::string file_name): _file_name(std::move(file_name)) { }

	friend
	auto to_string(const ResourceNotFound& self) -> std::string {
		return fmt::format("Resource file '{}' not found", self._file_name);
	}

	auto file_name() const noexcept -> const std::string& { return _file_name; }

private:
	std::string _file_name;
};

inline
auto try_get_resource_string(
	cmrc::embedded_filesystem fs,
	const std::string& file_name,
	DiagnosticSink& diag_sink
) -> Status<std::string> {
	const auto file_data = try_get_resource_data(fs, file_name);
	if (!file_data) {
		diag_sink(ResourceNotFound{file_name});
		return from_error;
	}
	return std::string(file_data->begin(), file_data->end());
}

} // namespace fr
#endif // include guard
