#ifndef FRACTAL_BOX_GRAPHICS_GL_MESH_HPP
#define FRACTAL_BOX_GRAPHICS_GL_MESH_HPP

#include <optional>
#include <span>
#include <vector>

#include <fmt/format.h>

#include "fractal_box/core/algorithm.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/error_handling/diagnostic.hpp"
#include "fractal_box/core/error_handling/status.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/graphics/gl_attributes.hpp"
#include "fractal_box/graphics/gl_common.hpp"

namespace fr {

inline constexpr GLsizei unknown_gl_vertex_count = -1;

class GlAttribBlock {
public:
	static constexpr
	GlAttribBlock planar(std::vector<GlAttribLabel> attribs) {
		return {GlAttribOrder::Planar, std::move(attribs)};
	}

	static constexpr
	GlAttribBlock interleaved(std::vector<GlAttribLabel> attribs) {
		return {GlAttribOrder::Interleaved, std::move(attribs)};
	}

	constexpr
	GlAttribBlock(
		GlAttribOrder attribOrder,
		std::vector<GlAttribLabel> attribs,
		GLsizei vertexCount = unknown_gl_vertex_count
	)
		: _attributes(std::move(attribs))
		, _order{attribOrder}
		, _count{vertexCount}
	{ }

	constexpr
	GlAttribBlock(const GlAttribLabel& attrib)
		: _attributes(1, attrib)
		, _order{GlAttribOrder::Interleaved} // doesn't matter which one
	{ }

	constexpr
	auto has_vertex_count() const noexcept -> bool { return *_count != unknown_gl_vertex_count; }

	/// @pre `hasVertexCount()`
	size_t calc_data_size() const noexcept;

	constexpr
	auto order() const noexcept -> GlAttribOrder { return _order.value(); }

	constexpr
	auto attribs() const noexcept -> const std::vector<GlAttribLabel>& { return _attributes; }

	constexpr
	auto count() const noexcept -> GLsizei { return _count.value(); }

	constexpr
	void setCount(GLsizei count) noexcept { _count = count; }

private:
	std::vector<GlAttribLabel> _attributes;
	WithDefaultValue<GlAttribOrder::Interleaved> _order;
	WithDefaultValue<unknown_gl_vertex_count> _count;
//	HashedCStrView name; // tag/label
};

/// @see https://www.khronos.org/opengl/wiki/Vertex_Specification_Best_Practices
class GlBufferLayout {
public:
	GlBufferLayout() = default;

	explicit constexpr
	GlBufferLayout(std::vector<GlAttribBlock> blocks):
		_blocks(std::move(blocks))
	{
#if FR_ASSERT_ENABLED
		for (auto& block : _blocks) {
			FR_ASSERT_CHECK(block.has_vertex_count());
		}
#endif
	}

	explicit constexpr
	GlBufferLayout(GLsizei vetexCount, std::vector<GlAttribBlock> blocks):
		_blocks(std::move(blocks))
	{
		for (auto& block : _blocks) {
			FR_ASSERT(!block.has_vertex_count());
			block.setCount(vetexCount);
		}
	}

	void clear() noexcept { _blocks.clear(); }

	constexpr const auto& blocks() const noexcept { return _blocks; }

private:
	std::vector<GlAttribBlock> _blocks;
};

namespace detail {

inline constexpr
auto find_gl_attrib_in_layout(
	const GlBufferLayout& layout, const GlAttribLabel& target
) noexcept -> const GlAttribLabel* {
	for (const auto& block : layout.blocks()) {
		for (const auto& attrib : block.attribs()) {
			if (attrib.name == target.name)
				return &attrib;
		}
	}
	return nullptr;
}

} // namespace detail

class MakeGlVertexBufferGenBuffersError: public ErrorBase {
public:
	explicit
	MakeGlVertexBufferGenBuffersError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(MakeGlVertexBufferGenBuffersError self) -> std::string {
		return fmt::format("Can't make GlVertexBuffer: failed to create array buffer object "
			"(OpenGL error '{}')", self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;
};

class MakeGlVertexBufferTooMuchDataError: public ErrorBase {
public:
	friend
	auto to_string(MakeGlVertexBufferTooMuchDataError) -> std::string {
		return "Can't make GlVertexBuffer: too much data";
	}
};

class MakeGlVertexBufferFailedToSendError: public ErrorBase {
public:
	explicit
	MakeGlVertexBufferFailedToSendError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(MakeGlVertexBufferFailedToSendError self) -> std::string {
		return fmt::format("Can't make GlVertexBuffer: failed to send data (OpenGL error '{}')",
			self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;
};

/// @brief OpenGL Vertex Buffer Object (VBO): a buffer bound to the `GL_ARRAY_BUFFER` target and
/// used to store vertex data
class GlVertexBuffer: public GlObject {
public:
	static constexpr GlObjectId null_native_id = 0;
	static constexpr GlBufferUsage default_usage = GlBufferUsage::DynamicDraw;

	static
	auto make(
		GlBufferLayout layout,
		GlBufferUsage usage,
		DiagnosticSink& error_sink
	) -> Status<GlVertexBuffer>;

	static
	auto make_from_raw_data(
		std::span<const std::byte> data,
		GlBufferLayout layout,
		GlBufferUsage usage,
		DiagnosticSink& error_sink
	) -> Status<GlVertexBuffer>;

	explicit constexpr
	GlVertexBuffer() noexcept = default;

	explicit
	GlVertexBuffer(
		AdoptInit,
		GlObjectId oid,
		GlBufferLayout layout,
		GlBufferUsage usage,
		size_t dataSize
	) noexcept;

	GlVertexBuffer(const GlVertexBuffer& other) = delete;
	GlVertexBuffer& operator=(const GlVertexBuffer& other) = delete;

	GlVertexBuffer(GlVertexBuffer&& other) noexcept = default;
	GlVertexBuffer& operator=(GlVertexBuffer&& other) noexcept;

	~GlVertexBuffer();

	friend
	void swap(GlVertexBuffer& lhs, GlVertexBuffer& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	static
	void bind_by_id(GlObjectId native_id) noexcept;

	void bind() noexcept;

	static
	void unbind() noexcept;

	bool isValid() const noexcept { return !_oid.is_default(); }

	const GlBufferLayout& layout() const noexcept { return _layout; }
	GlObjectId native_id() const noexcept { return *_oid; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	GlBufferLayout _layout;
	WithDefaultValue<default_usage> _usage;
	/// @brief Size of the GPU managed data in bytes
	WithDefault<size_t, 0> _data_size;
};

enum class GlIndexType: GLenum {
	UByte = GL_UNSIGNED_BYTE,
	UShort = GL_UNSIGNED_SHORT,
	UInt = GL_UNSIGNED_INT,
};

inline constexpr GlIndexType null_gl_indexType {};

inline constexpr
auto gl_index_type_size(GlIndexType type) noexcept -> size_t {
	using enum GlIndexType;
	switch (type) {
		case UByte: return sizeof(GLubyte);
		case UShort: return sizeof(GLushort);
		case UInt: return sizeof(GLuint);
	}
	FR_UNREACHABLE_MSG("Unknown GlIndexType");
}

inline
auto gl_index_offset_ptr(GlIndexType type, GLsizei firstIndex) noexcept -> const void* {
	FR_ASSERT(firstIndex > 0);
	return reinterpret_cast<const void*>(
		static_cast<GLintptr>(firstIndex) * static_cast<GLintptr>(gl_index_type_size(type))
	);
}

class MakeGlIndexBufferGenBuffersError: public ErrorBase {
public:
	explicit
	MakeGlIndexBufferGenBuffersError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(MakeGlIndexBufferGenBuffersError self) -> std::string {
		return fmt::format("Can't make GlVertexBuffer: failed to create array buffer object "
			"(OpenGL error '{}')", self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;
};

/// @brief OpenGL Element Buffer Object (EBO): a buffer bound to the `GL_ELEMENT_ARRAY_BUFFER`
/// target and used to store vertex indices
class GlIndexBuffer: public GlObject {
public:
	static constexpr GlObjectId null_native_id = 0;

	[[nodiscard]] static
	auto make(DiagnosticSink& error_sink) -> Status<GlIndexBuffer>;

	[[nodiscard]] static
	auto make_from_data(
		std::span<const std::byte> data,
		GlIndexType dataType,
		GLsizei vertexCount,
		DiagnosticSink& error_sink
	) -> Status<GlIndexBuffer>;

	explicit constexpr
	GlIndexBuffer() noexcept = default;

	explicit
	GlIndexBuffer(
		AdoptInit,
		GlObjectId oid,
		GlIndexType dataType,
		GLsizei vertexCount
	) noexcept;

	GlIndexBuffer(const GlIndexBuffer& other) = delete;
	GlIndexBuffer& operator=(const GlIndexBuffer& other) = delete;

	GlIndexBuffer(GlIndexBuffer&& other) noexcept = default;
	GlIndexBuffer& operator=(GlIndexBuffer&& other) noexcept;

	~GlIndexBuffer();

	friend
	void swap(GlIndexBuffer& lhs, GlIndexBuffer& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	void bind() noexcept;

	static
	void unbind() noexcept;

	bool isValid() const noexcept { return !_oid.is_default(); }

	GlObjectId native_id() const noexcept { return *_oid; }
	GlIndexType index_type() const noexcept { return *_data_type; }
	GLsizei vertex_count() const noexcept { return *_vertex_count; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	WithDefaultValue<GlIndexType{}> _data_type;
	WithDefaultValue<unknown_gl_vertex_count> _vertex_count;
};

class AttribIsNotUniqueError: public ErrorBase {
public:
	explicit
	AttribIsNotUniqueError(const char* name) noexcept: _name{name} { }

	friend
	auto to_string(AttribIsNotUniqueError self) -> std::string {
		return fmt::format("Attribute '{}' is not unique", self._name);
	}

private:
	const char* _name;
};

class AttribLocationMultiUseError: public ErrorBase {
public:
	explicit
	AttribLocationMultiUseError(GLuint location, const char* name1, const char* name2) noexcept:
		_location{location},
		_name1{name1},
		_name2{name2}
	{ }

	friend
	auto to_string(const AttribLocationMultiUseError& self) -> std::string {
		return fmt::format("Attribute location {} is used by multiple attributes ('{}', '{}')",
			self._location, self._name1, self._name2);
	}

	auto location() const noexcept -> GLuint { return _location; }
	auto name1() const noexcept -> const char* { return _name1; }
	auto name2() const noexcept -> const char* { return _name2; }

private:
	GLuint _location;
	const char* _name1;
	const char* _name2;
};

class VertexBufferIsNotUniqueError: public ErrorBase {
public:
	explicit
	VertexBufferIsNotUniqueError(GlObjectId id) noexcept: _id{id} { }

	friend
	auto to_string(VertexBufferIsNotUniqueError self) -> std::string {
		return fmt::format("Vertex buffer (object id {}) is not unique", self._id);
	}

	auto id() const noexcept -> GlObjectId { return _id; }

private:
	GlObjectId _id;
};

class AttribDoesntReferToBuffersError: public ErrorBase {
public:
	explicit
	AttribDoesntReferToBuffersError(const char* name, GlObjectId buffer_id) noexcept:
		_name{name},
		_buffer_id{buffer_id}
	{ }

	friend
	auto to_string(AttribDoesntReferToBuffersError self) -> std::string {
		return fmt::format("attribute '{}' doesn't refer to any of the buffers (buffer id {})",
			self._name, self._buffer_id);
	}

	auto name() const noexcept -> const char* { return _name; }
	auto buffer_id() const noexcept -> GlObjectId { return _buffer_id; }

private:
	const char* _name;
	GlObjectId _buffer_id;
};

class AttribIsntOwnedByBufferrsError: public ErrorBase {
public:
	explicit
	AttribIsntOwnedByBufferrsError(const char* name) noexcept:
		_name{name}
	{ }

	friend
	auto to_string(AttribIsntOwnedByBufferrsError self) -> std::string {
		return fmt::format("Attibute '{}' isn't owned by any of the buffers", self._name);
	}

	auto name() const noexcept -> const char* { return _name; }

private:
	const char* _name;
};

class AttribDoesntMatchError: public ErrorBase {
public:
	explicit
	AttribDoesntMatchError(const char* name, GlObjectId buffer_id) noexcept:
		_name{name},
		_buffer_id{buffer_id}
	{ }

	friend
	auto to_string(AttribDoesntMatchError self) -> std::string {
		return fmt::format("Attribute '{}' doesn't match the corresponding attribute stored in the "
			"buffer with object id {}", self._name, self._buffer_id);
	}

	auto name() const noexcept -> const char* { return _name; }
	auto buffer_id() const noexcept -> GlObjectId { return _buffer_id; }

private:
	const char* _name;
	GlObjectId _buffer_id;
};

/// See `Magnum::DynamicAttribute` (in Attribute.h:703)
struct GlMeshAttribInfo {
	static constexpr
	auto validate(
		std::span<const GlMeshAttribInfo> attributes,
		const auto& vertex_buffers,
		DiagnosticSink& error_sink
	) noexcept -> bool {
		bool success = true;
		// All attributes must be unique
		if (!test_all_unique_small_reported(attributes.begin(), attributes.end(), EqualTo<>{},
			[](const GlMeshAttribInfo& attrib) { return attrib.label.name; },
			[&error_sink] (const GlMeshAttribInfo& attrib, const GlMeshAttribInfo&) {
				error_sink(AttribIsNotUniqueError{attrib.label.name.str()});
			})
		) {
			success = false;
		}

		// All attribute locations must be unique
		if (!test_all_unique_small_reported(attributes.begin(), attributes.end(), EqualTo<>{},
			[](const GlMeshAttribInfo& attrib) { return attrib.location; },
			[&error_sink] (const GlMeshAttribInfo& attrib1, const GlMeshAttribInfo& attrib2) {
				error_sink(AttribLocationMultiUseError{attrib1.location,
					attrib1.label.name.str(), attrib2.label.name.str()});
			})
		) {
			success = false;
		}

		// TODO: Attribute stride can't be less than its size

		// All vertex buffers must be unique
		if (!test_all_unique_small_reported(vertex_buffers.begin(), vertex_buffers.end(),
			EqualTo<>{}, [](const auto& buffer) { return buffer->native_id(); },
			[&error_sink] (const auto& buffer, const auto&) {
				error_sink(VertexBufferIsNotUniqueError{buffer->native_id()});
			})
		) {
			success = false;
		}
		if (!success)
			return false;

		for (auto attrib : attributes) {
			const auto cmp_by_id = [&attrib](const auto& buffer) {
				return buffer->native_id() == attrib.array_buffer_id;
			};

			// Each attribute should refer to one of the buffers
			const auto buffer = std::ranges::find_if(vertex_buffers, cmp_by_id);
			if (buffer == vertex_buffers.end()) {
				error_sink(AttribDoesntReferToBuffersError{attrib.label.name.str(),
					attrib.array_buffer_id});
				success = false;
				continue;
			}

			// Each attribute should be stored within one of the buffers
			const auto* const buffer_attrib_label = detail::find_gl_attrib_in_layout(
				(*buffer)->layout(), attrib.label);
			if (!buffer_attrib_label) {
				error_sink(AttribIsntOwnedByBufferrsError{attrib.label.name.str()});
				success = false;
				continue;
			}

			if (*buffer_attrib_label != attrib.label) {
				error_sink(AttribDoesntMatchError{attrib.label.name.str(),
					(*buffer)->native_id()});
				success = false;
				continue;
			}
		}
		return success;
	}

public:
	GlAttribLabel label;
	GlObjectId array_buffer_id;
	GLuint location;
	GLintptr offset;
	GLsizei stride; // `DynamicAttribute::_vectorStride`
	// TODO: GLuint divisor;
};

/// @brief The range of vertices that should be drawn by some draw call
struct GlIndexRange {
	GLint first_index = 0;
	GLsizei count = 0;
};

class GenVertexArraysError: public ErrorBase {
public:
	explicit
	GenVertexArraysError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(GenVertexArraysError self) -> std::string {
		return fmt::format("Failed to create vertex array object (OpenGL error '{}')", self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;
};

class AttribBindError: public ErrorBase {
public:
	explicit
	AttribBindError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(AttribBindError self) -> std::string {
		return fmt::format("Failed to bind attrributes (OpenGL error '{}')", self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;

};

class IndexBufferBindError: public ErrorBase {
public:
	explicit
	IndexBufferBindError(GlErrorFlags error_flags) noexcept:
		_flags{error_flags}
	{ }

	friend
	auto to_string(IndexBufferBindError self) -> std::string {
		return fmt::format("Failed to bind index buffer (OpenGL error '{}')", self._flags);
	}

	auto flags() const noexcept -> GlErrorFlags { return _flags; }

private:
	GlErrorFlags _flags;

};

/// @brief OpenGL Vertex Array Object (VAO)
/// @note Not to be confused with `Model`, which represents a set of meshes, textures, etc.
class GlMesh: public GlObject {
public:
	static constexpr GlObjectId null_native_id = 0;

	struct Params {
		std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers;
		std::shared_ptr<GlIndexBuffer> indexBuffer;
	};

	static
	auto makeMatchByLabels(
		std::span<const GlShaderAttrib> attribs,
		std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers,
		std::shared_ptr<GlIndexBuffer> indexBuffer,
		IDiagnosticSink& error_sink
	) -> std::optional<GlMesh>;

	static
	auto make(
		GlPrimitive primitive,
		std::vector<std::shared_ptr<GlVertexBuffer>> vertex_buffers,
		std::vector<GlMeshAttribInfo> attributes,
		std::shared_ptr<GlIndexBuffer> index_buffer,
		GlIndexRange index_range,
		DiagnosticSink& error_sink
	) -> Status<GlMesh>;

	GlMesh(
		AdoptInit,
		GlObjectId native_id,
		GlPrimitive primitive,
		std::vector<std::shared_ptr<GlVertexBuffer>> vertex_buffers,
		std::shared_ptr<GlIndexBuffer> index_buffer,
		std::vector<GlMeshAttribInfo> attributes,
		GlIndexRange index_range
	) noexcept;

	GlMesh() = default;

	GlMesh(const GlMesh& other) = delete;
	GlMesh& operator=(const GlMesh& other) = delete;

	GlMesh(GlMesh&& other) noexcept = default;
	GlMesh& operator=(GlMesh&& other) noexcept;

	~GlMesh();

	friend
	void swap(GlMesh& lhs, GlMesh& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	void bind() noexcept;

	static
	void unbind() noexcept;

	bool isValid() const noexcept { return !_oid.is_default(); }

	GlIndexRange index_range() const noexcept { return *_index_range; }
	void set_index_range(GlIndexRange range) noexcept;

	void draw_with_bound_shader();

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	WithDefaultValue<GlPrimitive{}> _primitive;
	// TODO: Replace with a custom centralized resource manager system based on sparse sets
	// and reference counting
	std::vector<std::shared_ptr<GlVertexBuffer>> _vertex_buffers;
	std::shared_ptr<GlIndexBuffer> _index_buffer;
	std::vector<GlMeshAttribInfo> _attributes;
	WithDefaultValue<GlIndexRange{}> _index_range;
	WithDefaultValue<unknown_gl_vertex_count> _total_vertex_count;
	// TODO: base vertex
};

} // namespace fr
#endif
