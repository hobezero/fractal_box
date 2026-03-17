#ifndef FRACTAL_BOX_CORE_ERROR_HANDLING_ERROR_HPP
#define FRACTAL_BOX_CORE_ERROR_HANDLING_ERROR_HPP

#include <concepts>

#include <fmt/format.h>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @note In GCC might cause false-positive -Wdangling-reference warnings.
/// [GCC bugtracker](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107532)
template<std::convertible_to<bool> OptLike>
FR_FORCE_INLINE constexpr
auto unwrap(OptLike&& opt) noexcept -> decltype(auto) {
	FR_PANIC_CHECK(opt);
	return *std::forward<OptLike>(opt);
}

template<std::convertible_to<bool> OptLike>
FR_FORCE_INLINE constexpr
auto unwrap_msg(OptLike&& opt, std::string_view err_msg) noexcept -> decltype(auto) {
	FR_PANIC_CHECK_MSG(opt, err_msg);
	return *std::forward<OptLike>(opt);
}

template<std::convertible_to<bool> OptLike, class... Args>
FR_FORCE_INLINE constexpr
auto unwrap_fmt(
	OptLike&& opt, fmt::format_string<Args...> format, Args&&... args
) noexcept -> decltype(auto) {
	FR_PANIC_CHECK_FMT(opt, format, std::forward<Args>(args)...);
	return *std::forward<OptLike>(opt);
}

} // namespace fr
#endif // include guard
