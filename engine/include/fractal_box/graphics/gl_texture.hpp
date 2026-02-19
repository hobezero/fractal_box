#ifndef FR_GRAPHICS_GL_TEXTURE_HPP
#define FR_GRAPHICS_GL_TEXTURE_HPP

#include <cstddef>

#include <optional>
#include <span>

#include <glm/vec2.hpp>

#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/diagnostic.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/graphics/gl_common.hpp"

namespace fr {

enum class GlTextureType: GLenum {
	T1d = GL_TEXTURE_1D,
	T2d = GL_TEXTURE_2D,
	T2dMultisample = GL_TEXTURE_2D_MULTISAMPLE,
	T3d = GL_TEXTURE_3D,
	Cubemap = GL_TEXTURE_CUBE_MAP,

	T1dArray = GL_TEXTURE_1D_ARRAY,
	T2dArray = GL_TEXTURE_2D_ARRAY,
	T2dMultisampleArray = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
	CubemapArray = GL_TEXTURE_CUBE_MAP_ARRAY,

	Rectangle = GL_TEXTURE_RECTANGLE,
};

inline constexpr
auto to_string_view(GlTextureType type) noexcept -> std::string_view {
	using enum GlTextureType;
	switch (type) {
		case T1d: return "GL_TEXTURE_1D";
		case T2d: return "GL_TEXTURE_2D";
		case T2dMultisample: return "GL_TEXTURE_2D_MULTISAMPLE";
		case T3d: return "GL_TEXTURE_3D";
		case Cubemap: return "GL_TEXTURE_CUBE_MAP";
		case T1dArray: return "GL_TEXTURE_1D_ARRAY";
		case T2dArray: return "GL_TEXTURE_2D_ARRAY";
		case T2dMultisampleArray: return "GL_TEXTURE_2D_MULTISAMPLE_ARRAY";
		case CubemapArray: return "GL_TEXTURE_CUBE_MAP_ARRAY";
		case Rectangle: return "GL_TEXTURE_RECTANGLE";
	}
	FR_UNREACHABLE();
}

enum class GlTextureWrapping: GLint {
	Repeat = GL_REPEAT,
	MirroredRepeat = GL_MIRRORED_REPEAT,
	ClampToEdge = GL_CLAMP_TO_EDGE,

#if FR_TARGET_GL_DESKTOP
	ClampToBorder = GL_CLAMP_TO_BORDER,
	// GL_MIRROR_CLAMP_TO_EDGE is not available in OpenGL 4.3
	// MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
#endif
};

enum class GlTextureMinFiltering: GLint {
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR,
	NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	LinearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST,
	NearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR,
	LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR
};

enum class GlTextureMagFiltering: GLint {
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR,
};

/// @brief The type of components used to represent texture data on the GPU
/// @todo TODO: Conditionally disable formats not supported on OpenGL ES or WebGL
enum class GlTextureFormat: GLint {
	// Base
	Depth = GL_DEPTH_COMPONENT,
	Depth16 = GL_DEPTH_COMPONENT16,
	Depth24 = GL_DEPTH_COMPONENT24,
	Depth32F = GL_DEPTH_COMPONENT32F,
	DepthStencil = GL_DEPTH_STENCIL,
	Depth24Stencil8 = GL_DEPTH24_STENCIL8,
	Depth32Stencil8 = GL_DEPTH32F_STENCIL8,
	Red = GL_RED,
	RG = GL_RG,
	RGB = GL_RGB,
	RGBA = GL_RGBA,
	SRGB = GL_SRGB,
	SRGBA = GL_SRGB_ALPHA,
	// Size
	R8 = GL_R8,
	R8_SNorm = GL_R8_SNORM,
	R16 = GL_R16,
	R16_SNorm = GL_R16_SNORM,
	RG8 = GL_RG8,
	RG8_SNorm = GL_RG8_SNORM,
	RG16 = GL_RG16,
	RG16_SNorm = GL_RG16_SNORM,
	R3G3B2 = GL_R3_G3_B2,
	RGB4 = GL_RGB4,
	RGB5 = GL_RGB5,
	RGB8 = GL_RGB8,
	RGB8_SNorm = GL_RGB8_SNORM,
	RGB10 = GL_RGB10,
	RGB12 = GL_RGB12,
	RGB16 = GL_RGB16,
	RGB16_SNorm = GL_RGB16_SNORM,
	RGBA2 = GL_RGBA2,
	RGBA4 = GL_RGBA4,
	RGB5_A1 = GL_RGB5_A1,
	RGBA8 = GL_RGBA8,
	RGBA8_SNorm = GL_RGBA8_SNORM,
	RGB10_A2 = GL_RGB10_A2,
	RGB10_A2UI = GL_RGB10_A2UI,
	RGBA12 = GL_RGBA12,
	RGBA16 = GL_RGBA16,
	SRGB8 = GL_SRGB8,
	SRGB8_A8 = GL_SRGB8_ALPHA8,
	R16F = GL_R16F,
	RG16F = GL_RG16F,
	RGB16F = GL_RGB16F,
	RGBA16F = GL_RGBA16F,
	R32F = GL_R32F,
	RG32F = GL_RG32F,
	RGB32F = GL_RGB32F,
	RGBA32F = GL_RGBA32F,
	R11F_G11F_B10F = GL_R11F_G11F_B10F,
	RGB9_E5 = GL_RGB9_E5,
	R8I = GL_R8I,
	R8UI = GL_R8UI,
	R16I = GL_R16I,
	R16UI = GL_R16UI,
	R32I = GL_R32I,
	R32UI = GL_R32UI,
	RG8I = GL_RG8I,
	RG8UI = GL_RG8UI,
	RG16I = GL_RG16I,
	RG16UI = GL_RG16UI,
	RG32I = GL_RG32I,
	RG32UI = GL_RG32UI,
	RGB8I = GL_RGB8I,
	RGB8UI = GL_RGB8UI,
	RGB16I = GL_RGB16I,
	RGB16UI = GL_RGB16UI,
	RGB32I = GL_RGB32I,
	RGB32UI = GL_RGB32UI,
	RGBA8I = GL_RGBA8I,
	RGBA8UI = GL_RGBA8UI,
	RGBA16I = GL_RGBA16I,
	RGBA16UI = GL_RGBA16UI,
	RGBA32I = GL_RGBA32I,
	RGBA32UI = GL_RGBA32UI,
	// TODO: Compressed
};

/// @brief The component type used to represent source image data in RAM
/// @todo TODO: Conditionally disable formats not supported on OpenGL ES or WebGL
enum class GlPixelFormat: GLenum {
	Red = GL_RED,
	RG = GL_RG,
	RGB = GL_RGB,
	BGR = GL_BGR,
	RGBA = GL_RGBA,
	BGRA = GL_BGRA,
	Red_Integer = GL_RED_INTEGER,
	RG_Integer = GL_RG_INTEGER,
	RGB_Integer = GL_RGB_INTEGER,
	BGR_Integer = GL_BGR_INTEGER,
	RGBA_Integer = GL_RGBA_INTEGER,
	BGRA_Integer = GL_BGRA_INTEGER,
	Stencil = GL_STENCIL_INDEX,
	Depth = GL_DEPTH_COMPONENT,
	DepthStencil = GL_DEPTH_STENCIL,
};

/// @todo TODO: Conditionally disable data types not supported on OpenGL ES or WebGL
enum class GlPixelDataType: GLuint {
	UByte = GL_UNSIGNED_BYTE,
	Byte = GL_BYTE,
	UShort = GL_UNSIGNED_SHORT,
	Short = GL_SHORT,
	UInt = GL_UNSIGNED_INT,
	Int = GL_INT,
	Float = GL_FLOAT,
	UByte_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
	UByte_2_3_3_Rev = GL_UNSIGNED_BYTE_2_3_3_REV,
	UShort_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
	UShort_5_6_5_Rev = GL_UNSIGNED_SHORT_5_6_5_REV,
	UShort_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
	UShort_4_4_4_4_Rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
	UShort_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
	UShort_5_5_5_1_Rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,
	UInt_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
	UInt_8_8_8_8_Rev = GL_UNSIGNED_INT_8_8_8_8_REV,
	UInt_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
	UInt_2_10_10_10_Rev = GL_UNSIGNED_INT_2_10_10_10_REV,
	UInt_24_8 = GL_UNSIGNED_INT_24_8,
};

struct GlTextureParams {
	GlTextureFormat format = GlTextureFormat::RGBA8;

	GlTextureWrapping wrapping_s = GlTextureWrapping::Repeat;
	GlTextureWrapping wrapping_t = GlTextureWrapping::Repeat;

	GlTextureMinFiltering min_filtering = GlTextureMinFiltering::Linear;
	GlTextureMagFiltering mag_filtering = GlTextureMagFiltering::Linear;

	int num_mipmap_levels = 1;
	bool autogen_mipmaps = true;
};

class GlTexture2d {
public:
	static constexpr auto type_target = GlTextureType::T2d;
	static constexpr GlObjectId null_native_id = 0;

	GlTexture2d() = default;

	explicit
	GlTexture2d(
		AdoptInit,
		GlObjectId object_id,
		const GlTextureParams& params,
		glm::ivec2 dimensions
	) noexcept;

	static
	auto make(IDiagnosticSink& error_sink) noexcept -> std::optional<GlTexture2d>;

	static
	auto make_from_raw_data(
		const GlTextureParams& params,
		std::span<const std::byte> src_data,
		glm::ivec2 src_dimensions,
		GlPixelFormat src_format,
		GlPixelDataType src_data_type,
		IDiagnosticSink& error_sink
	) noexcept -> std::optional<GlTexture2d>;

	static
	auto make_empty(
		const GlTextureParams& params,
		glm::ivec2 src_dimensions,
		GlPixelFormat src_format,
		GlPixelDataType src_data_type,
		IDiagnosticSink& error_sink
	) noexcept -> std::optional<GlTexture2d>;

	GlTexture2d(const GlTexture2d& other) = delete;
	auto operator=(const GlTexture2d& other) -> GlTexture2d& = delete;

	GlTexture2d(GlTexture2d&& other) noexcept = default;
	auto operator=(GlTexture2d&& other) noexcept -> GlTexture2d&;

	~GlTexture2d();

	friend
	void swap(GlTexture2d& lhs, GlTexture2d& rhs) noexcept;

	[[nodiscard]]
	auto release() noexcept -> GlObjectId;

	void destroy() noexcept;

	static
	void bind_by_id(GlObjectId object_id) noexcept;

	void bind() const noexcept;

	static
	void unbind() noexcept;

	auto is_valid() const noexcept -> bool { return !_oid.is_default(); }

	auto native_id() const noexcept -> GlObjectId { return *_oid; }
	auto params() const noexcept -> const GlTextureParams& { return _params; }
	auto dimensions() const noexcept -> glm::ivec2 { return _dimensions; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	GlTextureParams _params;
	glm::ivec2 _dimensions;
};

} // namespace fr
#endif // include guard
