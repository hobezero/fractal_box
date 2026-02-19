#include "fractal_box/graphics/gl_mesh.hpp"

#include <algorithm>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/scope.hpp"

namespace fr {

size_t GlAttribBlock::calcDataSize() const noexcept {
	FR_ASSERT(hasVertexCount());
	size_t columnSize = 0;
	// TODO: Consider alignment
	for (const auto& label : _attributes)
		columnSize += gl_attrib_size(label.data_type, label.num_components);
	return columnSize * static_cast<size_t>(*_count);
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

auto GlVertexBuffer::makeFromRawData(
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
	buffer->_dataSize = data.size();
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
	, _dataSize{dataSize}
{ }

GlVertexBuffer& GlVertexBuffer::operator=(GlVertexBuffer&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_layout = std::move(other._layout);
	_usage = std::move(other._usage);
	_dataSize = std::move(other._dataSize);

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
	swap(lhs._dataSize, rhs._dataSize);
}

GlObjectId GlVertexBuffer::release() noexcept {
	_layout.clear();
	_usage.release();
	_dataSize.release();

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

void GlVertexBuffer::bindById(GlObjectId native_id) noexcept {
	glBindBuffer(GL_ARRAY_BUFFER, native_id);
}

void GlVertexBuffer::bind() noexcept {
	bindById(*_oid);
}

void GlVertexBuffer::unbind() noexcept {
	bindById(null_native_id);
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
	GlIndexType dataType,
	GLsizei vertexCount
) noexcept
	: _oid{oid}
	, _dataType{dataType}
	, _vertexCount{vertexCount}
{ }

GlIndexBuffer& GlIndexBuffer::operator=(GlIndexBuffer&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_dataType = std::move(other._dataType);
	_vertexCount = std::move(other._vertexCount);

	return *this;
}

GlIndexBuffer::~GlIndexBuffer() {
	do_destroy();
}

void swap(GlIndexBuffer& lhs, GlIndexBuffer& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._dataType, rhs._dataType);
	swap(lhs._vertexCount, rhs._vertexCount);
}

GlObjectId GlIndexBuffer::release() noexcept {
	_dataType.release();
	_vertexCount.release();

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
	std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers,
	std::vector<GlMeshAttribInfo> attributes,
	std::shared_ptr<GlIndexBuffer> indexBuffer,
	GlIndexRange indexRange,
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

	if (!GlMeshAttribInfo::validate(attributes, vertexBuffers, myErrors))
		return std::nullopt;

	auto mesh = GlMesh{
		adopt,
		oid,
		primitive,
		std::move(vertexBuffers),
		std::move(indexBuffer),
		std::move(attributes),
		indexRange
	};

	FR_DEFER [] {
		GlMesh::unbind();
		GlIndexBuffer::unbind();
		GlVertexBuffer::unbind();
	};
	mesh.bind();
	GlObjectId currentBuffer = GlVertexBuffer::null_native_id;
	for (const auto& attrib : mesh._attributes) {
		if (attrib.arrayBufferId != currentBuffer)
			GlVertexBuffer::bindById(attrib.arrayBufferId);
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

	if (mesh._indexBuffer) {
		mesh._indexBuffer->bind();
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
	std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers,
	std::shared_ptr<GlIndexBuffer> indexBuffer,
	std::vector<GlMeshAttribInfo> attributes,
	GlIndexRange indexRange
) noexcept
	: _oid{native_id}
	, _primitive{primitive}
	, _vertexBuffers(std::move(vertexBuffers))
	, _indexBuffer{std::move(indexBuffer)}
	, _attributes(std::move(attributes))
	, _indexRange{indexRange}
	, _totalVertexCount{indexBuffer ? indexBuffer->vertexCount() : unknown_gl_vertex_count}
{ }

GlMesh& GlMesh::operator=(GlMesh&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_primitive = std::move(other._primitive);
	_vertexBuffers = std::move(other._vertexBuffers);
	_indexBuffer = std::move(other._indexBuffer);
	_attributes = std::move(other._attributes);
	_indexRange = std::move(other._indexRange);
	_totalVertexCount = std::move(other._totalVertexCount);

	return *this;
}

GlMesh::~GlMesh() {
	do_destroy();
}

void swap(GlMesh& lhs, GlMesh& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._primitive, rhs._primitive);
	swap(lhs._vertexBuffers, rhs._vertexBuffers);
	swap(lhs._indexBuffer, rhs._indexBuffer);
	swap(lhs._attributes, rhs._attributes);
	swap(lhs._indexRange, rhs._indexRange);
	swap(lhs._totalVertexCount, rhs._totalVertexCount);
}

GlObjectId GlMesh::release() noexcept {
	_primitive = null_gl_primitive;
	_vertexBuffers.clear();
	_indexBuffer.reset();
	_attributes.clear();
	_indexRange = {};
	_totalVertexCount = unknown_gl_vertex_count;

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
	FR_ASSERT(range.firstIndex >= 0 && range.count > 0);
	FR_ASSERT(range.firstIndex + range.count <= _totalVertexCount);

	_indexRange = range;
}

void GlMesh::drawWithBoundShader() {
	FR_ASSERT(!_oid.is_default());

	bind();
	if (_indexBuffer && _indexBuffer->isValid()) {
		glDrawElements(
			to_underlying(*_primitive),
			_indexRange->count,
			to_underlying(_indexBuffer->indexType()),
			gl_index_offset_ptr(_indexBuffer->indexType(), _indexRange->firstIndex)
		);
		// glDrawRangeElements: same as glDrawElements, with a potential optimization
		// (vertex indices pointed by (indices, count) lie between [start, end] inclusive
	}
	else {
		glDrawArrays(to_underlying(*_primitive), _indexRange->firstIndex, _indexRange->count);
	}

	FR_GL_ASSERT_FAST();
}

} // namespace fr
