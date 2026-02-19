#ifndef FRACTAL_BOX_CORE_RELOCATION_HPP
#define FRACTAL_BOX_CORE_RELOCATION_HPP

/// @see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3516r2.html
/// @see https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3631r0.html
/// @see https://github.com/isidorostsa/hpx_fork/blob/d4a3a4a04ae0fa61624ecd5155241ea36efd8ac8/libs/core/type_support/include/hpx/type_support/uninitialized_relocate.hpp

#include <cstring>

#include <iterator>
#include <memory>
#include <new>
#include <utility>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T>
concept c_has_custom_is_trivially_relocatable = requires(T x) {
	{ kepler_custom_is_trivially_relocatable(x) } -> c_const_value_of_type<bool>;
};

template<class T>
inline constexpr auto is_trivially_relocatable = bool_c<std::is_trivially_copyable_v<T>>;

template<c_has_custom_is_trivially_relocatable T>
inline constexpr auto is_trivially_relocatable<T>
	= decltype(kepler_custom_is_trivially_relocatable(std::declval<T>())){};

template<class T>
concept c_trivially_relocatable = is_trivially_relocatable<T>();

/// @note `std::move_constructible` implies `std::destructible`
template<class T>
concept c_relocatable = std::move_constructible<T> || c_trivially_relocatable<T>;

template<class Dest, class Src>
concept c_relocatable_from = std::same_as<Dest, Src> && c_relocatable<Dest>;

template<std::move_constructible T>
inline constexpr
auto move_and_destroy(
	T* dest, T* src
) noexcept(std::is_nothrow_move_constructible_v<T>) -> T* {
	struct Guard {
		constexpr
		~Guard() { std::destroy_at(obj); }
	public:
		T* obj;
	};

	// Guard calls the destructor even in case of an exception thrown by the move constructor
	[[maybe_unused]] const auto guard = Guard{src};
	return std::construct_at(dest, std::move(*src));
}

template<class T>
concept c_nothrow_relocatable
	= c_trivially_relocatable<T> || (c_nothrow_move_constructible<T> && c_nothrow_destructible<T>);

template<c_trivially_relocatable T>
inline
auto trivially_relocate(T* src_begin, T* src_end, T* dest_begin) noexcept -> T* {
	const auto n = static_cast<size_t>(src_end - src_begin);
	// This is UB, but should work in every major compiler
	std::memmove(dest_begin, src_begin, sizeof(T) * n);
	return std::launder(dest_begin + n);
}

template<class T>
requires c_relocatable<T>
FR_FORCE_INLINE constexpr
auto relocate_at(T* dest, T* src) noexcept(c_nothrow_relocatable<T>) -> T* {
	if (dest == src)
		return dest;

	if constexpr (is_trivially_relocatable<T>) {
		if consteval {
			return move_and_destroy(dest, src);
		}
		else {
			return trivially_relocate(src, src + 1, dest);
		}
	}
	else {
		return move_and_destroy(dest, src);
	}
}

namespace detail {

template<class SrcIter, class DestIter>
FR_FORCE_INLINE
auto relocate_many_trivial(
	SrcIter src_begin, SrcIter src_end, DestIter dest_begin
) noexcept -> DestIter {
	const auto n = std::distance(src_begin, src_end);
	return trivially_relocate(
		std::to_address(src_begin),
		std::to_address(src_begin) + n,
		std::to_address(dest_begin)
	);
}

template<class SrcIter, class Size, class DestIter>
FR_FORCE_INLINE
auto relocate_many_n_trivial(
	SrcIter src_begin, Size n, DestIter dest_begin
) noexcept -> std::pair<SrcIter, DestIter> {
	const auto dest_end = trivially_relocate(
		std::to_address(src_begin),
		std::to_address(src_begin) + n,
		std::to_address(dest_begin)
	);
	return {src_begin + n, dest_end};
}

template<class SrcIter, class DestIter>
FR_FORCE_INLINE constexpr
auto relocate_many_nothrow(SrcIter src_begin, SrcIter src_end, DestIter dest_begin) noexcept {
	// TODO: Consider moving and destroying in bulk (`std::uninitialized_move` + `std::destroy`)
	for (; src_begin != src_end; static_cast<void>(++src_begin), static_cast<void>(++dest_begin)) {
		// Dereferencing iterators is safe since they are guaranteed to point to valid objects
		// (assuming precodintions have been met)
		relocate_at(std::to_address(dest_begin), std::to_address(src_begin));
	}
	return dest_begin;
}

template<class SrcIter, class Size, class DestIter>
FR_FORCE_INLINE constexpr
auto relocate_many_n_nothrow(
	SrcIter src_begin, Size n, DestIter dest_begin
) noexcept -> std::pair<SrcIter, DestIter> {
	for (auto i = n;
		i != Size{0};
		--i, static_cast<void>(++src_begin), static_cast<void>(++dest_begin)
	) {
		// Dereferencing iterators is safe since they are guaranteed to point to valid objects
		// (assuming precodintions have been met)
		relocate_at(std::to_address(dest_begin), std::to_address(src_begin));
	}
	return {std::move(src_begin), std::move(dest_begin)};
}

template<class SrcIter, class DestIter>
FR_FORCE_INLINE constexpr
auto relocate_many_throwing(SrcIter src_begin, SrcIter src_end, DestIter dest_begin) {
	auto dest_it = dest_begin;
	try {
		for (; src_begin != src_end; static_cast<void>(++src_begin), static_cast<void>(++dest_it)) {
			// Dereferencing iterators is safe since they are guaranteed to point to valid objects
			// (assuming precodintions have been met)
			relocate_at(std::to_address(dest_it), std::to_address(src_begin));
		}
	}
	catch (...) {
		++src_begin;
		std::destroy(src_begin, src_end);
		std::destroy(dest_begin, dest_it);
		throw;
	}
	return dest_it;
}

template<class SrcIter, class Size, class DestIter>
FR_FORCE_INLINE constexpr
auto relocate_many_n_throwing(
	SrcIter src_begin, Size n, DestIter dest_begin
) -> std::pair<SrcIter, DestIter> {
	auto dest_it = dest_begin;
	auto i = n;
	try {
		for (; i != Size{0}; --i, static_cast<void>(++src_begin), static_cast<void>(++dest_it)) {
			// Dereferencing iterators is safe since they are guaranteed to point to valid objects
			// (assuming precodintions have been met)
			relocate_at(std::to_address(dest_it), std::to_address(src_begin));
		}
	}
	catch (...) {
		++src_begin;
		--i;
		std::destroy_n(src_begin, i);
		std::destroy(dest_begin, dest_it);
		throw;
	}
	return {std::move(src_begin), std::move(dest_it)};
}

} // namespace detail

/// @see P1144, P3516 with some changes
/// @pre No overlap: `dest_begin` is not in the range `[src_begin, src_end)`
template<std::forward_iterator SrcIter, std::forward_iterator DestIter>
requires c_relocatable_from<std::iter_value_t<SrcIter>, std::iter_value_t<DestIter>>
FR_FORCE_INLINE constexpr
auto uninitialized_relocate(
	SrcIter src_begin, SrcIter src_end, DestIter dest_begin
) noexcept(c_nothrow_relocatable<std::iter_value_t<SrcIter>>) -> DestIter {
	using Value = std::iter_value_t<SrcIter>;
	if constexpr (is_trivially_relocatable<Value> && std::contiguous_iterator<SrcIter>) {
		if consteval {
			return detail::relocate_many_nothrow(std::move(src_begin), std::move(src_end),
				std::move(dest_begin));
		}
		else {
			return detail::relocate_many_trivial(std::move(src_begin), std::move(src_end),
				std::move(dest_begin));
		}
	}
	else if constexpr (c_nothrow_relocatable<Value>) {
		return detail::relocate_many_nothrow(std::move(src_begin), std::move(src_end),
			std::move(dest_begin));
	}
	else {
		return detail::relocate_many_throwing(std::move(src_begin), std::move(src_end),
			std::move(dest_begin));
	}
}

template<std::forward_iterator SrcIter, class Size, std::forward_iterator DestIter>
requires c_relocatable_from<std::iter_value_t<SrcIter>, std::iter_value_t<DestIter>>
FR_FORCE_INLINE constexpr
auto uninitialized_relocate_n(
	SrcIter src_begin, Size n, DestIter dest_begin
) noexcept(c_nothrow_relocatable<std::iter_value_t<SrcIter>>) -> std::pair<SrcIter, DestIter> {
	using Value = std::iter_value_t<SrcIter>;
	if constexpr (is_trivially_relocatable<Value> && std::contiguous_iterator<SrcIter>) {
		if consteval {
			return detail::relocate_many_n_nothrow(std::move(src_begin), n, std::move(dest_begin));
		}
		else {
			return detail::relocate_many_n_trivial(std::move(src_begin), n, std::move(dest_begin));
		}
	}
	else if constexpr (c_nothrow_relocatable<Value>) {
		return detail::relocate_many_n_nothrow(std::move(src_begin), n, std::move(dest_begin));
	}
	else {
		return detail::relocate_many_n_throwing(std::move(src_begin), n, std::move(dest_begin));
	}
}

/* TODO: uninitialized_relocate_backward. Might like something like this:

/// @pre No overlap: `out_result` is not in the range `[in_first, in_last)`
template<std::bidirectional_iterator InIter, std::bidirectional_iterator OutIter>
inline constexpr
auto uninitialized_relocate_backward(
	InIter in_first, InIter in_last, OutIter out_result
) -> OutIter {
	auto in_cursor = in_first;
	auto out_cursor = out_result;
	try {
		while (in_cursor != in_first) {
			--in_cursor;
			--out_cursor;
			relocate_at(std::addressof(*out_cursor), std::addressof(*in_cursor));
		}
	}
	catch (...) {
		std::destroy(in_cursor, in_last);
		++out_cursor;
		std::destroy(out_cursor, out_result);
	}
	return {in_cursor, out_cursor};
}
*/

} // namespace fr
#endif // include guard
