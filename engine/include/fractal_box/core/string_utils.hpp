#ifndef FRACTAL_BOX_CORE_STRING_UTILS_HPP
#define FRACTAL_BOX_CORE_STRING_UTILS_HPP

#include <cstring>

#include <iterator> // std::size
#include <type_traits>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/assert.hpp"

namespace fr {

namespace detail {

template<class Char>
inline constexpr
auto naive_str_length(const Char* str) noexcept -> size_t {
	auto len = 0uz;
	for (const auto* c = str; *c != Char{0}; ++c)
		++len;
	return len;
}

} // namespace detail

template<class T>
FR_FORCE_INLINE constexpr
auto str_length(const T& str) noexcept -> size_t {
	using Char = std::remove_cvref_t<decltype(str[0])>;
	if constexpr (std::is_array_v<T>) {
		FR_ASSERT_AUDIT_MSG(str[std::size(str) - 1zu] == Char{'\0'},
			"String must be null-terminated");
		return std::size(str) - 1zu;
	}
	else if consteval {
		return detail::naive_str_length(str);
	}
	else {
		if constexpr (std::is_same_v<Char, char>)
			return std::strlen(str);
		if constexpr (std::is_same_v<Char, wchar_t>)
			return std::wcslen(str);
		else
			return detail::naive_str_length(str);
	}
}

} // namespace fr
#endif // include guard
