#ifndef FRACTAL_BOX_GRAPHICS_GL_MESH_HPP
#define FRACTAL_BOX_GRAPHICS_GL_MESH_HPP

#include <optional>
#include <span>
#include <vector>

#include <fmt/format.h>

#include "fractal_box/core/algorithm.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/diagnostic.hpp"
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

	constexpr GlAttribBlock(const GlAttribLabel& attrib)
		: _attributes(1, attrib)
		, _order{GlAttribOrder::Interleaved} // doesn't matter which one
	{ }

	constexpr bool hasVertexCount() const noexcept { return *_count != unknown_gl_vertex_count; }

	/// @pre `hasVertexCount()`
	size_t calcDataSize() const noexcept;

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

	explicit constexpr GlBufferLayout(std::vector<GlAttribBlock> blocks)
		: _blocks(std::move(blocks))
	{
#if FR_ASSERT_ENABLED
		for (auto& block : _blocks) {
			FR_ASSERT_CHECK(block.hasVertexCount());
		}
#endif
	}

	explicit constexpr GlBufferLayout(GLsizei vetexCount, std::vector<GlAttribBlock> blocks)
		: _blocks(std::move(blocks))
	{
		for (auto& block : _blocks) {
			FR_ASSERT(!block.hasVertexCount());
			block.setCount(vetexCount);
		}
	}

	void clear() noexcept { _blocks.clear(); }

	constexpr const auto& blocks() const noexcept { return _blocks; }

private:
	std::vector<GlAttribBlock> _blocks;
};

namespace detail {

inline constexpr const GlAttribLabel* find_gl_attrib_in_layout(
	const GlBufferLayout& layout, const GlAttribLabel& target
) noexcept {
	for (const auto& block : layout.blocks()) {
		for (const auto& attrib : block.attribs()) {
			if (attrib.name == target.name)
				return &attrib;
		}
	}
	return nullptr;
}

} // namespace detail

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
		IDiagnosticSink& error_sink
	) -> std::optional<GlVertexBuffer>;

	static
	auto makeFromRawData(
		std::span<const std::byte> data,
		GlBufferLayout layout,
		GlBufferUsage usage,
		IDiagnosticSink& error_sink
	) -> std::optional<GlVertexBuffer>;

	explicit constexpr GlVertexBuffer() noexcept = default;
	explicit GlVertexBuffer(
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

	friend void swap(GlVertexBuffer& lhs, GlVertexBuffer& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	static void bindById(GlObjectId native_id) noexcept;
	void bind() noexcept;
	static void unbind() noexcept;

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
	WithDefault<size_t, 0> _dataSize;
};

enum class GlIndexType: GLenum {
	UByte = GL_UNSIGNED_BYTE,
	UShort = GL_UNSIGNED_SHORT,
	UInt = GL_UNSIGNED_INT,
};

inline constexpr GlIndexType nullGlIndexType {};

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
/// @brief OpenGL Element Buffer Object (EBO): a buffer bound to the `GL_ELEMENT_ARRAY_BUFFER`
/// target and used to store vertex indices
class GlIndexBuffer: public GlObject {
public:
	static constexpr GlObjectId null_native_id = 0;

	[[nodiscard]] static auto make(IDiagnosticSink& error_sink) -> std::optional<GlIndexBuffer>;
	[[nodiscard]] static auto makeFromData(
		std::span<const std::byte> data,
		GlIndexType dataType,
		GLsizei vertexCount,
		IDiagnosticSink& error_sink
	) -> std::optional<GlIndexBuffer>;

	explicit constexpr GlIndexBuffer() noexcept = default;
	explicit GlIndexBuffer(
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

	friend void swap(GlIndexBuffer& lhs, GlIndexBuffer& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	void bind() noexcept;
	static void unbind() noexcept;

	bool isValid() const noexcept { return !_oid.is_default(); }

	GlObjectId native_id() const noexcept { return *_oid; }
	GlIndexType indexType() const noexcept { return *_dataType; }
	GLsizei vertexCount() const noexcept { return *_vertexCount; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	WithDefaultValue<GlIndexType{}> _dataType;
	WithDefaultValue<unknown_gl_vertex_count> _vertexCount;
};

/// See `Magnum::DynamicAttribute` (in Attribute.h:703)
struct GlMeshAttribInfo {
	GlAttribLabel label;
	GlObjectId arrayBufferId;
	GLuint location;
	GLintptr offset;
	GLsizei stride; // `DynamicAttribute::_vectorStride`
	// TODO: GLuint divisor;

	static constexpr bool validate(
		std::span<const GlMeshAttribInfo> attributes,
		const auto& vertexBuffers,
		IDiagnosticSink& error_sink
	) noexcept {
		bool success = true;
		// All attributes should be unique
		if (!test_all_unique_small_reported(attributes.begin(), attributes.end(), EqualTo<>{},
			[](const GlMeshAttribInfo& attrib) { return attrib.label.name; },
			[&error_sink] (const GlMeshAttribInfo& attrib, const GlMeshAttribInfo&) {
				error_sink.push(fmt::format("attribute '{}' is not unique",
					attrib.label.name.str()));
			})
		) {
			success = false;
		}

		// All attribute locations should be unique
		if (!test_all_unique_small_reported(attributes.begin(), attributes.end(), EqualTo<>{},
			[](const GlMeshAttribInfo& attrib) { return attrib.location; },
			[&error_sink] (const GlMeshAttribInfo& attrib1, const GlMeshAttribInfo& attrib2) {
				error_sink.push(fmt::format("attribute location {} is used by multiple attributes "
					"('{}', '{}')", attrib1.location, attrib1.label.name.str(),
					attrib2.label.name.str()));
			})
		) {
			success = false;
		}

		// Attribute stride can't be less than its size

		// All vertex buffers should be unique
		if (!test_all_unique_small_reported(vertexBuffers.begin(), vertexBuffers.end(), EqualTo<>{},
			[](const auto& buffer) { return buffer->native_id(); },
			[&error_sink] (const auto& buffer, const auto&) {
				error_sink.push(fmt::format("vertex buffer (object id {}) is not unique",
					buffer->native_id()));
			})
		) {
			success = false;
		}
		if (!success)
			return false;

		for (auto attrib : attributes) {
			const auto cmpById = [&attrib](const auto& buffer) {
				return buffer->native_id() == attrib.arrayBufferId;
			};

			// Each attribute should refer to one of the buffers
			const auto buffer = std::find_if(std::begin(vertexBuffers), std::end(vertexBuffers),
				cmpById);
			if (buffer == vertexBuffers.end()) {
				error_sink.push(fmt::format("attribute '{}' doesn't refer to any of the buffers "
					"(buffer id {})", attrib.label.name.str(), attrib.arrayBufferId));
				success = false;
				continue;
			}

			// Each attribute should be stored within one of the buffers
			const auto* const bufferAttribLabel = detail::find_gl_attrib_in_layout((*buffer)->layout(),
				attrib.label);
			if (!bufferAttribLabel) {
				error_sink.push(fmt::format("attribute '{}' isn't owned by any of the buffers",
					attrib.label.name.str()));
				success = false;
				continue;
			}

			if (*bufferAttribLabel != attrib.label) {
				error_sink.push(fmt::format("attribute '{}' doesn't match the corresponding "
					"attribute stored in the buffer with object id {}", attrib.label.name.str(),
					(*buffer)->native_id()));
				success = false;
				continue;
			}
		}
		return success;
	}
};

/// @brief The range of vertices that should be drawn by some draw call
struct GlIndexRange {
	GLint firstIndex = 0;
	GLsizei count = 0;
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
		std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers,
		std::vector<GlMeshAttribInfo> attributes,
		std::shared_ptr<GlIndexBuffer> indexBuffer,
		GlIndexRange indexRange,
		IDiagnosticSink& error_sink
	) -> std::optional<GlMesh>;

	GlMesh(
		AdoptInit,
		GlObjectId native_id,
		GlPrimitive primitive,
		std::vector<std::shared_ptr<GlVertexBuffer>> vertexBuffers,
		std::shared_ptr<GlIndexBuffer> indexBuffer,
		std::vector<GlMeshAttribInfo> attributes,
		GlIndexRange indexRange
	) noexcept;

	GlMesh() = default;

	GlMesh(const GlMesh& other) = delete;
	GlMesh& operator=(const GlMesh& other) = delete;

	GlMesh(GlMesh&& other) noexcept = default;
	GlMesh& operator=(GlMesh&& other) noexcept;

	~GlMesh();

	friend void swap(GlMesh& lhs, GlMesh& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	void bind() noexcept;
	static void unbind() noexcept;

	bool isValid() const noexcept { return !_oid.is_default(); }

	GlIndexRange indexRange() const noexcept { return *_indexRange; }
	void setIndexRange(GlIndexRange range) noexcept;

	void drawWithBoundShader();

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	WithDefaultValue<GlPrimitive{}> _primitive;
	// TODO: Replace with a custom centralized resource manager system based on sparse sets
	// and reference counting
	std::vector<std::shared_ptr<GlVertexBuffer>> _vertexBuffers;
	std::shared_ptr<GlIndexBuffer> _indexBuffer;
	std::vector<GlMeshAttribInfo> _attributes;
	WithDefaultValue<GlIndexRange{}> _indexRange;
	WithDefaultValue<unknown_gl_vertex_count> _totalVertexCount;
	// TODO: base vertex
};

} // namespace fr
#endif
