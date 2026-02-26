#include "fractal_box/graphics/gl_mesh.hpp"

#include <algorithm>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/scope.hpp"

namespace fr {

size_t GlAttribBlock::calc_data_size() const noexcept {
	FR_ASSERT(has_vertex_count());
	size_t column_size = 0;
	// TODO: Consider alignment
	for (const auto& label : _attributes)
		column_size += gl_attrib_size(label.data_type, label.num_components);
	return column_size * static_cast<size_t>(*_count);
}

auto GlVertexBuffer::make(
	GlBufferLayout layout,
	GlBufferUsage usage,
	IDiagnosticSink& error_sink
) -> std::optional<GlVertexBuffer> {
	GlObjectId oid = null_native_id;
	glGenBuffers(1, &oid);
	if (const auto error_flags = get_all_gl_error_flags()) {
		error_sink.push(fmt::format("Can't make GlVertexBuffer: failed to create "
			"array buffer object (OpenGL error '{}')", error_flags));
		return std::nullopt;
	}
	return GlVertexBuffer{adopt, oid, std::move(layout), usage, 0};
}

auto GlVertexBuffer::make_from_raw_data(
	std::span<const std::byte> data,
	GlBufferLayout layout,
	GlBufferUsage usage,
	IDiagnosticSink& error_sink
) -> std::optional<GlVertexBuffer> {
	auto buffer = make(std::move(layout), usage, error_sink);
	if (!buffer)
		return buffer;

	buffer->bind();
	FR_DEFER [] { GlVertexBuffer::unbind(); };

	if (data.size() > size_t{std::numeric_limits<GLsizeiptr>::max()}) {
		error_sink.push("Can't make GlVertexBuffer: too much data");
		buffer = std::nullopt; // Make NRVO happy
		return buffer;
	}
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size()), data.data(),
		to_underlying(usage));
	if (const auto error_flags = get_all_gl_error_flags()) {
		error_sink.push(fmt::format("Can't make GlVertexBuffer: failed to send data "
			"(OpenGL error '{}')", error_flags));
		buffer = std::nullopt;
		return buffer;
	}
	buffer->_data_size = data.size();
	// TODO: Verify that data size corresponds to the number of vertices
	// TODO: handle GL_OUT_OF_MEMORY

	return buffer;
}

GlVertexBuffer::GlVertexBuffer(
	AdoptInit,
	GlObjectId oid,
	GlBufferLayout layout,
	GlBufferUsage usage,
	size_t dataSize
) noexcept
	: _oid{oid}
	, _layout{std::move(layout)}
	, _usage{usage}
	, _data_size{dataSize}
{ }

GlVertexBuffer& GlVertexBuffer::operator=(GlVertexBuffer&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_layout = std::move(other._layout);
	_usage = std::move(other._usage);
	_data_size = std::move(other._data_size);

	return *this;
}

GlVertexBuffer::~GlVertexBuffer() {
	do_destroy();
}

void swap(GlVertexBuffer& lhs, GlVertexBuffer& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._layout, rhs._layout);
	swap(lhs._usage, rhs._usage);
	swap(lhs._data_size, rhs._data_size);
}

GlObjectId GlVertexBuffer::release() noexcept {
	_layout.clear();
	_usage.release();
	_data_size.release();

	return _oid.release();
}

void GlVertexBuffer::destroy() noexcept {
	if (!_oid.is_default()) {
		do_destroy();
		release();
	}
}

void GlVertexBuffer::do_destroy() noexcept {
	if (!_oid.is_default()) {
		glDeleteBuffers(1, &_oid.value());
	}
}

void GlVertexBuffer::bind_by_id(GlObjectId native_id) noexcept {
	glBindBuffer(GL_ARRAY_BUFFER, native_id);
}

void GlVertexBuffer::bind() noexcept {
	bind_by_id(*_oid);
}

void GlVertexBuffer::unbind() noexcept {
	bind_by_id(null_native_id);
}

auto GlIndexBuffer::make(IDiagnosticSink& error_sink) -> std::optional<GlIndexBuffer> {
	GlObjectId oid = null_native_id;
	glGenBuffers(1, &oid);
	if (oid == null_native_id) {
		error_sink.push("Can't make GlIndexBuffer: failed to create index buffer object");
		return std::nullopt;
	}

	return GlIndexBuffer{adopt, oid, nullGlIndexType, 0};
}

GlIndexBuffer::GlIndexBuffer(
	AdoptInit,
	GlObjectId oid,
	GlIndexType data_type,
	GLsizei vertex_count
) noexcept
	: _oid{oid}
	, _data_type{data_type}
	, _vertex_count{vertex_count}
{ }

GlIndexBuffer& GlIndexBuffer::operator=(GlIndexBuffer&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_data_type = std::move(other._data_type);
	_vertex_count = std::move(other._vertex_count);

	return *this;
}

GlIndexBuffer::~GlIndexBuffer() {
	do_destroy();
}

void swap(GlIndexBuffer& lhs, GlIndexBuffer& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._data_type, rhs._data_type);
	swap(lhs._vertex_count, rhs._vertex_count);
}

GlObjectId GlIndexBuffer::release() noexcept {
	_data_type.release();
	_vertex_count.release();

	return _oid.release();
}

void GlIndexBuffer::destroy() noexcept {
	if (_oid != null_native_id) {
		do_destroy();
		release();
	}
}

void GlIndexBuffer::do_destroy() noexcept {
	if (_oid != null_native_id) {
		glDeleteBuffers(1, &_oid.value());
	}
}

void GlIndexBuffer::bind() noexcept {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *_oid);
}

void GlIndexBuffer::unbind() noexcept {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, null_native_id);
}

auto GlMesh::make(
	GlPrimitive primitive,
	std::vector<std::shared_ptr<GlVertexBuffer>> vertex_buffers,
	std::vector<GlMeshAttribInfo> attributes,
	std::shared_ptr<GlIndexBuffer> index_buffer,
	GlIndexRange index_range,
	IDiagnosticSink& error_sink
) -> std::optional<GlMesh> {
	DiagnosticSinkSlice myErrors{error_sink, [](const Diagnostic& error) {
		return fmt::format("Can't make GlMesh: {}", error);
	}};

	GlObjectId oid = null_native_id;
	glGenVertexArrays(1, &oid);
	if (const auto error_flags = get_all_gl_error_flags()) {
		myErrors.push(fmt::format("failed to create vertex array object (OpenGL error '{}')",
			error_flags));
		return std::nullopt;
	}

	if (!GlMeshAttribInfo::validate(attributes, vertex_buffers, myErrors))
		return std::nullopt;

	auto mesh = GlMesh{
		adopt,
		oid,
		primitive,
		std::move(vertex_buffers),
		std::move(index_buffer),
		std::move(attributes),
		index_range
	};

	FR_DEFER [] {
		GlMesh::unbind();
		GlIndexBuffer::unbind();
		GlVertexBuffer::unbind();
	};
	mesh.bind();
	GlObjectId current_buffer = GlVertexBuffer::null_native_id;
	for (const auto& attrib : mesh._attributes) {
		if (attrib.array_buffer_id != current_buffer)
			GlVertexBuffer::bind_by_id(attrib.array_buffer_id);
		glEnableVertexAttribArray(attrib.location);
		glVertexAttribPointer(
			attrib.location,
			attrib.label.num_components,
			to_underlying(attrib.label.data_type),
			static_cast<GLboolean>(attrib.label.is_normalized),
			attrib.stride,
			reinterpret_cast<void*>(attrib.offset)
		);
	}

	if (const auto error_flags = get_all_gl_error_flags()) {
		myErrors.push(fmt::format("failed to bind attributes (OpenGL error '{}')", error_flags));
		return std::nullopt;
	}

	if (mesh._index_buffer) {
		mesh._index_buffer->bind();
	}

	if (const auto error_flags = get_all_gl_error_flags()) {
		myErrors.push(fmt::format("failed to bind index buffer (OpenGL error '{}')", error_flags));
		return std::nullopt;
	}

	return mesh;
}

GlMesh::GlMesh(AdoptInit,
	GlObjectId native_id,
	GlPrimitive primitive,
	std::vector<std::shared_ptr<GlVertexBuffer>> vertex_buffers,
	std::shared_ptr<GlIndexBuffer> index_buffer,
	std::vector<GlMeshAttribInfo> attributes,
	GlIndexRange index_range
) noexcept
	: _oid{native_id}
	, _primitive{primitive}
	, _vertex_buffers(std::move(vertex_buffers))
	, _index_buffer{std::move(index_buffer)}
	, _attributes(std::move(attributes))
	, _index_range{index_range}
	, _total_vertex_count{index_buffer ? index_buffer->vertexCount() : unknown_gl_vertex_count}
{ }

GlMesh& GlMesh::operator=(GlMesh&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_primitive = std::move(other._primitive);
	_vertex_buffers = std::move(other._vertex_buffers);
	_index_buffer = std::move(other._index_buffer);
	_attributes = std::move(other._attributes);
	_index_range = std::move(other._index_range);
	_total_vertex_count = std::move(other._total_vertex_count);

	return *this;
}

GlMesh::~GlMesh() {
	do_destroy();
}

void swap(GlMesh& lhs, GlMesh& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._primitive, rhs._primitive);
	swap(lhs._vertex_buffers, rhs._vertex_buffers);
	swap(lhs._index_buffer, rhs._index_buffer);
	swap(lhs._attributes, rhs._attributes);
	swap(lhs._index_range, rhs._index_range);
	swap(lhs._total_vertex_count, rhs._total_vertex_count);
}

GlObjectId GlMesh::release() noexcept {
	_primitive = null_gl_primitive;
	_vertex_buffers.clear();
	_index_buffer.reset();
	_attributes.clear();
	_index_range = {};
	_total_vertex_count = unknown_gl_vertex_count;

	return _oid.release();
}

void GlMesh::destroy() noexcept {
	if (!_oid.is_default()) {
		do_destroy();
		release();
	}
}

void GlMesh::do_destroy() noexcept {
	if (!_oid.is_default()) {
		glDeleteVertexArrays(1, &_oid.value());
	}
}

void GlMesh::bind() noexcept {
	glBindVertexArray(*_oid);
}

void GlMesh::unbind() noexcept {
	glBindVertexArray(null_native_id);
}

void GlMesh::setIndexRange(GlIndexRange range) noexcept {
	FR_ASSERT(range.first_index >= 0 && range.count > 0);
	FR_ASSERT(range.first_index + range.count <= _total_vertex_count);

	_index_range = range;
}

void GlMesh::drawWithBoundShader() {
	FR_ASSERT(!_oid.is_default());

	bind();
	if (_index_buffer && _index_buffer->isValid()) {
		glDrawElements(
			to_underlying(*_primitive),
			_index_range->count,
			to_underlying(_index_buffer->indexType()),
			gl_index_offset_ptr(_index_buffer->indexType(), _index_range->first_index)
		);
		// glDrawRangeElements: same as glDrawElements, with a potential optimization
		// (vertex indices pointed by (indices, count) lie between [start, end] (inclusive)
	}
	else {
		glDrawArrays(to_underlying(*_primitive), _index_range->first_index, _index_range->count);
	}

	FR_GL_ASSERT_FAST();
}

} // namespace fr
