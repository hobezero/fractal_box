#ifndef FRACTAL_BOX_GRAPHICS_GL_ATTRIBUTES_HPP
#define FRACTAL_BOX_GRAPHICS_GL_ATTRIBUTES_HPP

#include "fractal_box/core/hashing/hashed_string.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/graphics/gl_common.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace fr {

enum class GlPrimitive: GLenum {
	Points = GL_POINTS,
	LineStrip = GL_LINE_STRIP,
	LineLoop = GL_LINE_LOOP,
	Lines = GL_LINES,
	LineStripAdjency = GL_LINE_STRIP_ADJACENCY,
	LinesAdjacency= GL_LINES_ADJACENCY,
	TriangleStrip = GL_TRIANGLE_STRIP,
	TriangleFan = GL_TRIANGLE_FAN,
	Triangles = GL_TRIANGLES,
	TriangleStripAdjacency = GL_TRIANGLE_STRIP_ADJACENCY,
	TrianglesAdjacency = GL_TRIANGLES_ADJACENCY,
	Patches = GL_PATCHES,
};

/// @note Can't use 0 as a null since it's already used by `GL_POINTS`
inline constexpr auto null_gl_primitive = static_cast<GlPrimitive>(static_cast<GLenum>(-1));

enum class GlBufferUsage: GLenum {
	StaticDraw = GL_STATIC_DRAW,
	DynamicDraw = GL_DYNAMIC_DRAW,
	StreamDraw = GL_STREAM_DRAW,
};

/// See `Magnum::DynamicAttribute::Kind` (in Attribute.h)
enum class GlAttribKind: uint8_t {
	Generic,
	GenericNormalized,
	Integral,
#if FR_TARGET_GL_DESKTOP
	Long,
#endif
};

/// See `Magnum::DynamicAttribute::DataType` (in Attribute.h)
enum class GlComponentType: GLenum {
	Byte = GL_BYTE,
	UByte = GL_UNSIGNED_BYTE,
	Short = GL_SHORT,
	UShort = GL_UNSIGNED_SHORT,
	Int = GL_INT,
	UInt = GL_UNSIGNED_INT,

	HalfFloat = GL_HALF_FLOAT,
	Float = GL_FLOAT,
#if FR_TARGET_GL_DESKTOP
	Double = GL_DOUBLE,

	UInt10f_11f_11fRev = GL_UNSIGNED_INT_10F_11F_11F_REV,
#endif
	Int2_10_10_10Rev = GL_INT_2_10_10_10_REV,
	Uint2_10_10_10Rev = GL_UNSIGNED_INT_2_10_10_10_REV,
};

inline constexpr
auto gl_component_type_size(GlComponentType type) noexcept -> size_t {
	using enum GlComponentType;
	switch (type) {
		case Byte: return sizeof(GLbyte);
		case UByte: return sizeof(GLubyte);
		case Short: return sizeof(GLshort);
		case UShort: return sizeof(GLushort);
		case Int: return sizeof(GLint);
		case UInt: return sizeof(GLuint);

		// TODO: how do we represent 16-bit floats and other exotic types?
		case HalfFloat: return sizeof(GLfloat) / 2u;
		case Float: return sizeof(GLfloat);
#if FR_TARGET_GL_DESKTOP
		case Double: return sizeof(GLdouble);

		case UInt10f_11f_11fRev: return sizeof(uint32_t);
#endif
		case Int2_10_10_10Rev: return sizeof(uint32_t);
		case Uint2_10_10_10Rev: return sizeof(uint32_t);
	}
	FR_UNREACHABLE();
}

namespace detail {

template<class T>
inline consteval
auto cpp_type_to_gl_component_type() -> GlComponentType {
	if constexpr (std::is_same_v<T, GLbyte>)
		return GlComponentType::Byte;
	if constexpr (std::is_same_v<T, GLshort>)
		return GlComponentType::Short;
	if constexpr (std::is_same_v<T, GLushort>)
		return GlComponentType::UShort;
	if constexpr (std::is_same_v<T, GLint>)
		return GlComponentType::Int;
	if constexpr (std::is_same_v<T, GLuint>)
		return GlComponentType::UInt;
	if constexpr (std::is_same_v<T, GLubyte>)
		return GlComponentType::UByte;
	if constexpr (std::is_same_v<T, GLfloat>)
		return GlComponentType::Float;
	FR_UNREACHABLE();
}

} // namespace detail

template<class T>
struct GlAttribTypeTraits;

template<glm::length_t Size, class T, glm::qualifier Q>
struct GlAttribTypeTraits<glm::vec<Size, T, Q>> {
	static constexpr auto default_component_type = detail::cpp_type_to_gl_component_type<T>();
	static constexpr auto num_components = static_cast<GLint>(Size);
};

/// @brief A "compound" attribute type means that components have different size
/// @details Usually, to calculate the total vertex size, we need to multiply the component size by
/// the number of components, except when the attribute (component) type is compound
inline constexpr
auto is_gl_component_type_compound(GlComponentType type) noexcept -> bool {
	using enum GlComponentType;
	switch (type) {
#if FR_TARGET_GL_DESKTOP
		case UInt10f_11f_11fRev:
#endif
		case Int2_10_10_10Rev:
		case Uint2_10_10_10Rev:
			return true;
		default:
			return false;
	}
	FR_UNREACHABLE_MSG("Unknown GlComponentType");
}

inline constexpr
auto gl_attrib_size(GlComponentType type, GLint num_components) noexcept -> size_t {
	const auto component_size = gl_component_type_size(type);
	return is_gl_component_type_compound(type)
		? component_size
		: component_size * static_cast<size_t>(num_components);
}

// TODO: handle non-default attribute index

enum class GlAttribOrder: uint8_t {
	Interleaved,
	Planar,
};

struct GlAttribLabel {
	template<class Vertex>
	static constexpr
	auto make(
		HashedCStrView label_name,
		bool is_attr_normalized = false,
		GlAttribKind attr_kind = GlAttribKind::Generic
	) noexcept -> GlAttribLabel {
		return {
			.name = label_name,
			.data_type = GlAttribTypeTraits<Vertex>::default_component_type,
			.num_components = GlAttribTypeTraits<Vertex>::num_components,
			.is_normalized = is_attr_normalized,
			.kind = attr_kind
		};
	}

	friend
	auto operator==(const GlAttribLabel& lhs, const GlAttribLabel& rhs) -> bool = default;

public:
	HashedCStrView name;
	GlComponentType data_type;
	GLint num_components;
	bool is_normalized = false;
	GlAttribKind kind = GlAttribKind::Generic;
};

struct GlShaderAttrib: GlAttribLabel {
	GLuint location;

	friend
	auto operator==(const GlShaderAttrib& lhs, const GlShaderAttrib& rhs) -> bool = default;
};

} // namespace fr
#endif // include guard
