#ifndef FRACTAL_BOX_GRAPHICS_GL_FRAMEBUFFER_HPP
#define FRACTAL_BOX_GRAPHICS_GL_FRAMEBUFFER_HPP

#include <optional>

#include <fmt/format.h>

#include "fractal_box/core/containers/linear_flat_set.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/error.hpp"
#include "fractal_box/graphics/gl_common.hpp"
#include "fractal_box/graphics/gl_texture.hpp"

namespace fr {

enum class GlRenderbufferFormat: GLenum {
	// Base
	Depth = GL_DEPTH_COMPONENT,
	Depth16 = GL_DEPTH_COMPONENT16,
	Depth24 = GL_DEPTH_COMPONENT24,
	Depth32 = GL_DEPTH_COMPONENT32,
	Depth32F = GL_DEPTH_COMPONENT32F,
	StencilIndex = GL_STENCIL_INDEX,
	StencilIndex1 = GL_STENCIL_INDEX1,
	StencilIndex4 = GL_STENCIL_INDEX4,
	StencilIndex8 = GL_STENCIL_INDEX8,
	StencilIndex16 = GL_STENCIL_INDEX16,
	DepthStencil = GL_DEPTH_STENCIL,
	Depth24Stencil8 = GL_DEPTH24_STENCIL8,
	Depth32Stencil8 = GL_DEPTH32F_STENCIL8,
	Red = GL_RED,
	RG = GL_RG,
	// RGB = GL_RGB,
	RGBA = GL_RGBA,
	// SRGB = GL_SRGB,
	// SRGBA = GL_SRGB_ALPHA,
	// // Size
	R8 = GL_R8,
	// R8_SNorm = GL_R8_SNORM,
	R16 = GL_R16,
	// R16_SNorm = GL_R16_SNORM,
	RG8 = GL_RG8,
	// RG8_SNorm = GL_RG8_SNORM,
	RG16 = GL_RG16,
	// RG16_SNorm = GL_RG16_SNORM,
	// R3G3B2 = GL_R3_G3_B2,
	// RGB4 = GL_RGB4,
	// RGB5 = GL_RGB5,
	// RGB8 = GL_RGB8,
	// RGB8_SNorm = GL_RGB8_SNORM,
	// RGB10 = GL_RGB10,
	// RGB12 = GL_RGB12,
	RGB16 = GL_RGB16,
	// RGB16_SNorm = GL_RGB16_SNORM,
	// RGBA2 = GL_RGBA2,
	RGBA4 = GL_RGBA4,
	RGB5_A1 = GL_RGB5_A1,
	RGBA8 = GL_RGBA8,
	// RGBA8_SNorm = GL_RGBA8_SNORM,
	RGB10_A2 = GL_RGB10_A2,
	RGB10_A2UI = GL_RGB10_A2UI,
	// RGBA12 = GL_RGBA12,
	RGBA16 = GL_RGBA16,
	// SRGB8 = GL_SRGB8,
	SRGB8_A8 = GL_SRGB8_ALPHA8,
	R16F = GL_R16F,
	RG16F = GL_RG16F,
	// RGB16F = GL_RGB16F,
	RGBA16F = GL_RGBA16F,
	R32F = GL_R32F,
	RG32F = GL_RG32F,
	// RGB32F = GL_RGB32F,
	RGBA32F = GL_RGBA32F,
	R11F_G11F_B10F = GL_R11F_G11F_B10F,
	// RGB9_E5 = GL_RGB9_E5,
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
	// RGB8I = GL_RGB8I,
	// RGB8UI = GL_RGB8UI,
	// RGB16I = GL_RGB16I,
	// RGB16UI = GL_RGB16UI,
	// RGB32I = GL_RGB32I,
	// RGB32UI = GL_RGB32UI,
	RGBA8I = GL_RGBA8I,
	RGBA8UI = GL_RGBA8UI,
	RGBA16I = GL_RGBA16I,
	RGBA16UI = GL_RGBA16UI,
	RGBA32I = GL_RGBA32I,
	RGBA32UI = GL_RGBA32UI,

	RGB565 = GL_RGB565,
};

class GlRenderbuffer {
public:
	static constexpr auto null_native_id = GlObjectId{0};

	static
	auto make() noexcept -> ErrorOr<GlRenderbuffer>;

	static
	auto make_with_storage(
		GlRenderbufferFormat format,
		glm::ivec2 dimensions
	) noexcept -> ErrorOr<GlRenderbuffer>;

	GlRenderbuffer() = default;

	explicit
	GlRenderbuffer(AdoptInit, GlObjectId oid) noexcept;

	GlRenderbuffer(const GlRenderbuffer&) = delete;
	auto operator=(const GlRenderbuffer&) -> GlRenderbuffer = delete;

	GlRenderbuffer(GlRenderbuffer&& other) noexcept = default;
	auto operator=(GlRenderbuffer&& other) noexcept -> GlRenderbuffer&;

	~GlRenderbuffer();

	void swap(GlRenderbuffer& other) noexcept;

	friend
	void swap(GlRenderbuffer& lhs, GlRenderbuffer& rhs) noexcept { lhs.swap(rhs); }

	[[nodiscard]]
	auto release() noexcept -> GlObjectId;

	void destroy();

	// static
	// void bind_by_id(GlObjectId object_id) noexcept;

	void bind() const noexcept;

	static
	void unbind() noexcept;

	/// @todo TODO: Rename to `has_value` or `initialized` (?)
	auto is_valid() const noexcept -> bool { return !_oid.is_default(); }

	auto native_id() const noexcept -> GlObjectId { return *_oid; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
};

enum class GlAttachmentKind {
	Renderbuffer,
	Texture,
};

inline constexpr
auto to_string_view(GlAttachmentKind kind) {
	using enum GlAttachmentKind;
	switch (kind) {
		case Renderbuffer: return "Renderbuffer";
		case Texture: return "Texture";
	}
	FR_UNREACHABLE();
}

class GlAttachmentPoint {
public:
	static constexpr
	auto depth() noexcept -> GlAttachmentPoint {
		return GlAttachmentPoint{adopt, GL_DEPTH_ATTACHMENT};
	}

	static constexpr
	auto stencil() noexcept -> GlAttachmentPoint {
		return GlAttachmentPoint{adopt, GL_STENCIL_ATTACHMENT};
	}

	static constexpr
	auto depth_stencil() noexcept -> GlAttachmentPoint {
		return GlAttachmentPoint{adopt, GL_DEPTH_STENCIL_ATTACHMENT};
	}

	static constexpr
	auto color(unsigned idx) noexcept -> GlAttachmentPoint {
		return GlAttachmentPoint{adopt, static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + idx)};
	}

	GlAttachmentPoint() = default;

	explicit FR_FORCE_INLINE constexpr
	GlAttachmentPoint(AdoptInit, GLenum point) noexcept: _value{point} { }

	auto operator<=>(this GlAttachmentPoint, GlAttachmentPoint) = default;

	constexpr
	auto is_depth() const noexcept -> bool {
		return _value == GL_DEPTH_ATTACHMENT;
	}

	constexpr
	auto has_depth() const noexcept -> bool {
		return _value == GL_DEPTH_ATTACHMENT || _value == GL_DEPTH_STENCIL_ATTACHMENT;
	}

	constexpr
	auto is_stencil() const noexcept -> bool {
		return _value == GL_STENCIL_ATTACHMENT;
	}

	constexpr
	auto has_stencil() const noexcept -> bool {
		return _value == GL_STENCIL_ATTACHMENT || _value == GL_DEPTH_STENCIL_ATTACHMENT;
	}

	constexpr
	auto is_depth_stencil() const noexcept -> bool {
		return _value == GL_DEPTH_STENCIL_ATTACHMENT;
	}

	constexpr
	auto is_color() const noexcept -> bool {
		return _value != GL_DEPTH_ATTACHMENT
			&& _value != GL_STENCIL_ATTACHMENT
			&& _value != GL_DEPTH_STENCIL_ATTACHMENT;
	}

	constexpr
	auto color_idx() -> std::optional<unsigned> {
		if (is_color())
			return static_cast<unsigned>(_value - GL_COLOR_ATTACHMENT0);
		return std::nullopt;
	}

	constexpr
	auto value() const noexcept { return _value; }

private:
	GLenum _value = 0;
};

class GlFramebuffer {
public:
	static constexpr auto null_native_id = GlObjectId{0};

	static
	auto make() noexcept -> ErrorOr<GlFramebuffer>;

	GlFramebuffer() = default;

	explicit
	GlFramebuffer(AdoptInit, GlObjectId oid) noexcept;

	GlFramebuffer(const GlFramebuffer&) = delete;
	auto operator=(const GlFramebuffer&) -> GlFramebuffer = delete;

	GlFramebuffer(GlFramebuffer&& other) noexcept = default;
	auto operator=(GlFramebuffer&& other) noexcept -> GlFramebuffer&;

	~GlFramebuffer();

	void swap(GlFramebuffer& other) noexcept;

	friend
	void swap(GlFramebuffer& lhs, GlFramebuffer& rhs) noexcept { lhs.swap(rhs); }

	[[nodiscard]]
	auto release() noexcept -> GlObjectId;

	void destroy();

	auto attach(GlAttachmentPoint point, const GlRenderbuffer& renderbuffer) -> ErrorOr<>;
	auto attach(GlAttachmentPoint point, const GlTexture2d& texture) -> ErrorOr<>;

	auto complete() -> ErrorOr<>;

	static
	void bind_default();

	/// @brief Bind for both read and draw access
	void bind() const noexcept;
	void bind_read() const noexcept;
	void bind_draw() const noexcept;

	static
	void unbind() noexcept;

	// TODO: Color space, Blending, Blitting, ...

	/// @todo TODO: Rename to `has_value` or `initialized` (?)
	auto is_valid() const noexcept -> bool { return !_oid.is_default(); }

	auto native_id() const noexcept -> GlObjectId { return *_oid; }

private:
	void do_destroy() noexcept;

private:
	struct Attachment {
		GlAttachmentPoint point;
		GlAttachmentKind kind;
		GlObjectId object_id;
	};

	WithDefaultValue<null_native_id> _oid;
	// TODO: Replace with LinearFlatMap or std::flat_map (once available)
	LinearFlatSet<Attachment, MemberEqualTo<&Attachment::point>> _attachments;
};

} // namespace fr

template<>
struct fmt::formatter<fr::GlAttachmentPoint>: formatter<fmt::string_view> {
	auto format(fr::GlAttachmentPoint point, auto& ctx) const {
		using Base = formatter<fmt::string_view>;
		if (point.is_depth())
			return Base::format("DEPTH", ctx);
		else if (point.is_stencil())
			return Base::format("STENCIL", ctx);
		else if (point.is_depth_stencil())
			return Base::format("DEPTH_STENCIL", ctx);
		else if (auto idx = point.color_idx())
			return fmt::format_to(ctx.out(), "COLOR{}", *idx);
		else
			FR_UNREACHABLE();
	}
};

template<>
struct fmt::formatter<fr::GlAttachmentKind>: formatter<fmt::string_view> {
	auto format(fr::GlAttachmentKind kind, auto& ctx) const {
		return formatter<fmt::string_view>::format(fr::to_string_view(kind), ctx);
	}
};

#endif
