#ifndef FRACTAL_BOX_CORE_IO_SPAN_IO_HPP
#define FRACTAL_BOX_CORE_IO_SPAN_IO_HPP

#include <algorithm>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/io/io_concepts.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/range_concepts.hpp"

namespace fr {

template<c_io_character Ch>
class SpanWriter {
public:
	using CharType = Ch;
	static constexpr auto is_buffered = true;
	static constexpr auto is_buffer_resizable = false;

	explicit FR_FORCE_INLINE constexpr
	SpanWriter(std::span<CharType> buffer) noexcept: _buffer(buffer) { }

	template<c_contiguous_container Cont>
	explicit constexpr
	SpanWriter(Cont& buffer) noexcept:
		_buffer(std::ranges::data(buffer), std::ranges::size(buffer))
	{ }

	template<size_t N>
	explicit FR_FORCE_INLINE constexpr
	SpanWriter(CharType (&arr)[N]) noexcept: _buffer(arr, N) { }

	SpanWriter(const SpanWriter&) = delete;
	auto operator=(const SpanWriter&) -> SpanWriter& = delete;

	SpanWriter(SpanWriter&&) = default;
	auto operator=(SpanWriter&&) -> SpanWriter& = default;

	~SpanWriter() = default;

	constexpr
	auto write(std::span<const CharType> data) noexcept -> Result<size_t, BufferOverrun> {
		if (_buffer.size() < data.size())
			return from_error;

		// TODO: Replace with memcpy: https://godbolt.org/z/b3cq6z3no
		std::ranges::copy(data, _buffer.begin());
		_buffer = _buffer.subspan(data.size());
		return data.size();
	}

	FR_FORCE_INLINE constexpr
	void flush() noexcept { }

	FR_FORCE_INLINE constexpr
	auto buffer() noexcept -> std::span<CharType> { return _buffer; }

	constexpr
	void commit_buffer(size_t size) noexcept {
		FR_ASSERT(size <= _buffer.size());
		_buffer = _buffer.subspan(size);
	}

private:
	std::span<CharType> _buffer;
};

template<class Ch>
SpanWriter(std::span<Ch>) -> SpanWriter<Ch>;

template<c_contiguous_container Cont>
SpanWriter(Cont&) -> SpanWriter<std::remove_reference_t<std::ranges::range_reference_t<Cont>>>;

template<class Ch, size_t N>
SpanWriter(Ch (&)[N]) -> SpanWriter<Ch>;

template<c_io_character Ch>
struct SpanReader {
	using CharType = Ch;
	static constexpr auto is_buffered = true;

	explicit FR_FORCE_INLINE constexpr
	SpanReader(std::span<const CharType> buffer) noexcept: _buffer{buffer} { }

	template<std::ranges::contiguous_range R>
	explicit constexpr
	SpanReader(R&& range) noexcept:
		_buffer(std::ranges::data(range), std::ranges::size(range))
	{ }

	template<size_t N>
	explicit FR_FORCE_INLINE constexpr
	SpanReader(const CharType (&arr)[N]) noexcept: _buffer(arr, N) { }

	SpanReader(const SpanReader&) = delete;
	auto operator=(const SpanReader&) -> SpanReader& = delete;

	SpanReader(SpanReader&&) = default;
	auto operator=(SpanReader&&) -> SpanReader& = default;

	~SpanReader() = default;

	constexpr
	auto read(std::span<CharType> data) noexcept -> Result<size_t, BufferOverrun> {
		if (_buffer.empty())
			return from_error;

		const auto chunk_size = std::min(_buffer.size(), data.size());
		std::ranges::copy(_buffer.subspan(0zu, chunk_size), data.begin());
		_buffer = _buffer.subspan(chunk_size);
		return chunk_size;
	}

	constexpr
	auto read_exact(std::span<CharType> data) noexcept -> Result<void, BufferOverrun> {
		if (_buffer.size() < data.size())
			return from_error;

		std::ranges::copy(_buffer.subspan(0zu, data.size()), data.begin());
		_buffer = _buffer.subspan(data.size());

		return {};
	}

	FR_FORCE_INLINE constexpr
	auto buffer() noexcept -> std::span<const CharType> { return _buffer; }

	constexpr
	void commit_buffer(size_t size) noexcept {
		FR_ASSERT(size <= _buffer.size());
		_buffer = _buffer.subspan(size);
	}

private:
	std::span<const CharType> _buffer;
};

template<class Ch>
SpanReader(std::span<Ch>) -> SpanReader<Ch>;

template<std::ranges::contiguous_range R>
SpanReader(R&&) -> SpanReader<std::remove_cvref_t<std::ranges::range_reference_t<R>>>;

template<class Ch, size_t N>
SpanReader(const Ch (&)[N]) -> SpanReader<Ch>;

} // namespace fr
#endif // include guard
