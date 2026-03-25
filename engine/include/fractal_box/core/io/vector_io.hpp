#ifndef FRACTAL_BOX_CORE_IO_VECTOR_IO_HPP
#define FRACTAL_BOX_CORE_IO_VECTOR_IO_HPP

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/range_concepts.hpp"

namespace fr {

template<c_vector_like Vec>
class VectorWriter {
public:
	using CharType = typename Vec::value_type;
	static constexpr auto is_buffered = true;
	static constexpr auto is_buffer_resizable = true;

	explicit FR_FORCE_INLINE constexpr
	VectorWriter(Vec& buffer) noexcept:
		_out{&buffer},
		_pos{buffer.size()}
	{ }

	VectorWriter(const VectorWriter&) = delete;
	auto operator=(const VectorWriter&) -> VectorWriter& = delete;

	VectorWriter(VectorWriter&&) = default;
	auto operator=(VectorWriter&&) -> VectorWriter& = default;

	~VectorWriter() = default;

	constexpr
	auto write(std::span<const CharType> data) -> size_t {
		if (data.empty())
			return 0zu;

		const auto pos_it = _out->begin() + static_cast<typename Vec::difference_type>(_pos);
		_out->erase(pos_it, _out->end());
		_out->insert_range(pos_it, data);
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
	void commit_buffer(size_t size) noexcept {
		FR_ASSERT(_pos + size <= _out->size());
		_pos += size;
	}

	constexpr
	auto resize_buffer(size_t size) -> size_t {
		if (_pos + size <= _out->size())
			return _out->size() - _pos;

		_out->resize(_pos + size);
		return size;
	}

private:
	Vec* _out;
	size_t _pos;
};

template<class Vec>
VectorWriter(Vec&) -> VectorWriter<Vec>;

} // namespace fr
#endif // include guard
