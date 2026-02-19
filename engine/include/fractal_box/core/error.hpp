#ifndef FRACTAL_BOX_CORE_ERROR_HPP
#define FRACTAL_BOX_CORE_ERROR_HPP

#include <concepts>
#include <expected>
#include <string>

#include <fmt/format.h>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
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

enum class Errc: int {
	Success = 0,
	UnknownError,
	OpenGlError,
	SdlError,
	GladError,
	ImGuiError,
	ResourceLoadingError,
};

inline constexpr
auto to_return_code(Errc errc) noexcept -> int {
	// Unix systems like to ignore the higher bits of return code
	const auto raw = to_underlying(errc);
	return raw > 0xFF ? 0xFF : raw;
}

class Error {
public:
	explicit
	Error(Errc code, std::string message):
		_code{code},
		_message(std::move(message))
	{ }

	auto code() const noexcept -> Errc { return _code; }
	auto message() const noexcept -> const std::string& { return _message; }

private:
	Errc _code;
	std::string _message;
};

template<class T = void>
using ErrorOr = std::expected<T, Error>;

template<class T, class Err>
concept c_expected_of = c_specialization<T, std::expected>
	&& std::same_as<typename T::error_type, Err>;

inline
auto make_error(Errc code, std::string message) -> std::unexpected<Error> {
	return std::unexpected<Error>{std::in_place, code, std::move(message)};
}

template<class... Args>
FR_FORCE_INLINE
auto make_error_fmt(
	Errc code, fmt::format_string<Args...> fmt_str, Args&&... args
) -> std::unexpected<Error> {
	return std::unexpected<Error>{std::in_place, code,
		fmt::format(fmt_str, std::forward<Args>(args)...)};
}

template<class T>
FR_FORCE_INLINE constexpr
auto extract_unexpected(T&& result) {
	using ErrType = typename std::remove_cvref_t<T>::error_type;
	return std::unexpected<ErrType>{std::forward<T>(result).error()};
}

} // namespace fr

template <>
struct fmt::formatter<fr::Error>: public fmt::formatter<std::string> {
	auto format(const fr::Error& error, auto& ctx) const {
		return fmt::formatter<std::string>::format(error.message(), ctx);
	}
};

#endif // include guard
