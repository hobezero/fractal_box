#ifndef FRACTAL_BOX_CORE_IO_STRING_IO_HPP
#define FRACTAL_BOX_CORE_IO_STRING_IO_HPP

#include <algorithm>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/range_concepts.hpp"

namespace fr {

template<c_string_like Str>
class StringWriter {
public:
	using CharType = typename Str::value_type;
	static constexpr auto is_buffered = true;
	static constexpr auto is_buffer_resizable = true;

	explicit FR_FORCE_INLINE constexpr
	StringWriter(Str& out) noexcept:
		_out{&out},
		_pos{out.size()}
	{ }

	StringWriter(const StringWriter&) = delete;
	auto operator=(const StringWriter&) -> StringWriter& = delete;

	StringWriter(StringWriter&&) = default;
	auto operator=(StringWriter&&) -> StringWriter& = default;

	~StringWriter() = default;

	constexpr
	auto write(std::span<const CharType> data) -> size_t {
		if (data.empty())
			return 0zu;

		_out->resize_and_overwrite(_pos + data.size(), [&](CharType* buf, size_t n) {
			std::ranges::copy(data, buf + _pos);
			return n;
		});
		_pos += data.size();
		return data.size();
	}

	FR_FORCE_INLINE constexpr
	void flush() noexcept { }

	FR_FORCE_INLINE constexpr
	auto buffer() noexcept -> std::span<CharType> {
		return {_out->data() + _pos, _out->size() - _pos};
	}

	constexpr
	auto commit_buffer(size_t size) -> size_t {
		FR_ASSERT(_pos + size <= _out->size());
		_pos += size;
		return size;
	}

	constexpr
	auto resize_buffer(size_t size) -> size_t {
		if (_pos + size <= _out->size())
			return _out->size() - _pos;

		_out->resize_and_overwrite(_pos + size, [](CharType*, size_t n) { return n; });
		return _out->size() - _pos;
	}

private:
	Str* _out;
	size_t _pos;
};

template<class Str>
StringWriter(Str&) -> StringWriter<Str>;


} // namespace fr
#endif // include guard
