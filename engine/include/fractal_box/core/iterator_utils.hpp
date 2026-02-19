#ifndef FRACTAL_BOX_CORE_ITERATOR_UTILS_HPP
#define FRACTAL_BOX_CORE_ITERATOR_UTILS_HPP

#include <iterator>
#include <memory>
#include <ranges>
#include <utility>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class Container>
requires std::contiguous_iterator<decltype(std::ranges::begin(std::declval<Container&>()))>
FR_FORCE_INLINE
auto end_ptr(Container&& c) noexcept {
	return std::ranges::data(c) + std::ranges::size(c);
}

/// @brief A small utility to enable proxy iterators to provide `operator->()`
/// @remarks Inspired by https://quuxplusone.github.io/blog/2019/02/06/arrow-proxy/
template<class Reference>
class ArrowProxy {
public:
	template<class U>
	explicit constexpr
	ArrowProxy(U&& ref) noexcept:
		_ref{std::forward<U>(ref)}
	{ }

	FR_FORCE_INLINE
	auto operator->() noexcept -> Reference* {
		return std::addressof(_ref);
	}

private:
	Reference _ref;
};

template<std::input_iterator Iterator>
struct IterResult {
	constexpr
	IterResult(Iterator it, bool ok) noexcept: where{it}, success{ok} { }

	FR_FORCE_INLINE constexpr
	auto value_unchecked() const noexcept -> std::iter_reference_t<Iterator> {
		return *this->where;
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return this->success; }

public:
	Iterator where {};
	bool success = false;
};

template<std::input_iterator Iterator>
struct InsertIterResult {
	constexpr
	InsertIterResult(Iterator it, bool ok) noexcept: where(it), success{ok} { }

	FR_FORCE_INLINE constexpr
	auto value() const noexcept -> std::iter_reference_t<Iterator> {
		return *this->where;
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return this->success; }

public:
	Iterator where {};
	bool success = false;
};

template<class T>
struct InsertValueResult {
	constexpr
	InsertValueResult(T& object, bool ok) noexcept:
		where{std::addressof(object)},
		success{ok}
	{ }

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return this->success; }

public:
	T* where = nullptr;
	bool success = false;
};

template<class Range, std::input_iterator Iterator>
FR_FORCE_INLINE constexpr
auto assert_iter(Range&& r, Iterator it) noexcept -> std::iter_reference_t<Iterator>
requires std::sentinel_for<decltype(std::ranges::end(r)), Iterator> {
	FR_ASSERT(it != std::ranges::end(r));
	return *it;
}

template<class Range, std::input_iterator Iterator>
FR_FORCE_INLINE constexpr
auto unwrap_iter(Range&& r, Iterator it) noexcept -> std::iter_reference_t<Iterator>
requires std::sentinel_for<decltype(std::ranges::end(r)), Iterator> {
	FR_PANIC_CHECK(it != std::ranges::end(r));
	return *it;
}

} // namespace fr
#endif // include guard
