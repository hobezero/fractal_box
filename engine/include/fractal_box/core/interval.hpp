#ifndef FRACTAL_BOX_CORE_INTERVAL_HPP
#define FRACTAL_BOX_CORE_INTERVAL_HPP

#include <ranges>
#include <span>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<c_arithmetic T>
class IntervalClosedClosed {
public:
	IntervalClosedClosed() = default;

	constexpr
	IntervalClosedClosed(T from, T to) noexcept:
		_from(from),
		_to(to)
	{
		FR_ASSERT(from <= to);
	}

	constexpr auto from() const noexcept -> T { return _from; }
	constexpr auto to() const noexcept -> T { return _to; }

	constexpr
	auto contains(T x) const noexcept -> bool {
		return _from <= x && x <= _to;
	}

private:
	T _from {};
	T _to {};
};

template<c_arithmetic T>
using IntervalClosed = IntervalClosedClosed<T>;

template<c_arithmetic T>
class IntervalClosedOpen {
public:
	using value_type = T;

	IntervalClosedOpen() = default;

	/// @note `from == to` represents an empty interval
	constexpr
	IntervalClosedOpen(T from, T to) noexcept:
		_from(from),
		_to(to)
	{
		FR_ASSERT(from <= to);
	}

	constexpr auto from() const noexcept -> T { return _from; }
	constexpr auto to() const noexcept -> T { return _to; }

	constexpr
	auto contains(T x) const noexcept -> bool {
		return _from <= x && x < _to;
	}

private:
	T _from {};
	T _to {};
};

template<c_arithmetic T>
class IntervalOpenOpen {
public:
	using value_type = T;

	IntervalOpenOpen() = default;

	/// @note `from == to` represents an empty interval
	constexpr
	IntervalOpenOpen(T from, T to) noexcept:
		_from(from),
		_to(to)
	{
		FR_ASSERT(from <= to);
	}

	constexpr auto from() const noexcept -> T { return _from; }
	constexpr auto to() const noexcept -> T { return _to; }

	constexpr
	auto contains(T x) const noexcept -> bool {
		return _from < x && x < _to;
	}

private:
	T _from {};
	T _to {};
};

template<c_arithmetic T>
using IntervalOpen = IntervalOpenOpen<T>;

class IndexInterval {
public:
	constexpr IndexInterval() = default;

	static constexpr
	auto from_sized(size_t from, size_t count) noexcept -> IndexInterval {
		return {from, count};
	}

	template<std::random_access_iterator It>
	static constexpr
	auto from_offset_iters(It begin, It start, It stop) -> IndexInterval {
		FR_ASSERT(start >= begin);
		FR_ASSERT(stop >= start);
		return {static_cast<size_t>(start - begin), static_cast<size_t>(stop - start)};
	}

	static constexpr
	auto from_all_range(std::ranges::sized_range auto&& source)
	noexcept(noexcept(std::ranges::size(source))) -> IndexInterval {
		return {0, std::ranges::size(source)};
	}

	static constexpr
	auto from_idxs(size_t from, size_t to) noexcept -> IndexInterval {
		FR_ASSERT(to >= from);
		return {from, to - from};
	}

	constexpr auto size() const noexcept -> size_t { return _size; }
	constexpr auto empty() const noexcept -> bool { return _size == 0; }

	constexpr auto from() const noexcept -> size_t { return _from; }
	constexpr auto to() const noexcept -> size_t { return _from + _size; }

	template<std::ranges::random_access_range R>
	constexpr
	auto apply(R&& source) const {
		using Difference = std::ranges::range_difference_t<R>;
		std::ranges::subrange{
			std::ranges::begin(source) + static_cast<Difference>(_from),
			std::ranges::begin(source) + static_cast<Difference>(_from + _size),
			_size
		};
	}

	constexpr
	auto apply(std::ranges::contiguous_range auto&& source) const noexcept {
		return std::span{std::ranges::data(source) + _from, _size};
	}

private:
	FR_FORCE_INLINE constexpr
	IndexInterval(size_t from, size_t size) noexcept:
		_from(from),
		_size(size)
	{ }

private:
	/// @brief Index of the first element in a range
	size_t _from = 0;
	/// @brief Index of the one-past-last element in a range
	size_t _size = 0;
};

} // namespace fr
#endif // include guard
