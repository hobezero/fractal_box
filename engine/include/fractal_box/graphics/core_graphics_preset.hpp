#ifndef FRACTAL_BOX_GRAPHICS_CORE_GRAPHICS_PRESET_HPP
#define FRACTAL_BOX_GRAPHICS_CORE_GRAPHICS_PRESET_HPP

#include <cmrc/cmrc.hpp>

#include "fractal_box/graphics/gl_framebuffer.hpp"
#include "fractal_box/graphics/gl_mesh.hpp"
#include "fractal_box/graphics/gl_texture.hpp"
#include "fractal_box/runtime/runtime.hpp"

// TODO: Move mesh/texture utils out of this file

namespace fr {

struct CoreMeshes {
	static
	auto make() -> ErrorOr<CoreMeshes>;

public:
	GlMesh ndc_quad;
};

/// @todo TODO: Ban mutable access to members
class Offscreen {
public:
	auto try_init(glm::ivec2 dimensions, IDiagnosticSink& errors) -> ErrorOr<>;

	auto depth_stencil_buffer() const noexcept -> const GlRenderbuffer& {
		return _depth_stencil_buffer;
	}

	auto color_texture() const noexcept -> const GlTexture2d& { return _color_texture; }

	auto framebuffer() const noexcept -> const GlFramebuffer& { return _framebuffer; }

private:
	GlRenderbuffer _depth_stencil_buffer;
	GlTexture2d _color_texture;
	GlFramebuffer _framebuffer;
};

struct CoreGraphicsPreset {
	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif
