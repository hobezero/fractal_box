#include "fractal_box/graphics/core_graphics_preset.hpp"

#include "fractal_box/graphics/core_shaders.hpp"
#include "fractal_box/graphics/gl_framebuffer.hpp"
#include "fractal_box/graphics/mesh_utils.hpp"
#include "fractal_box/platform/sdl_preset.hpp"
#include "fractal_box/runtime/core_preset.hpp"

namespace fr {

static constexpr UvVertex ndc_quad_data[] = {
	{{-1.f, +1.f}, {0.f, 1.f}},
	{{-1.f, -1.f}, {0.f, 0.f}},
	{{+1.f, -1.f}, {1.f, 0.f}},

	{{-1.f, +1.f}, {0.f, 1.f}},
	{{+1.f, -1.f}, {1.f, 0.f}},
	{{+1.f, +1.f}, {1.f, 1.f}},
};

auto CoreMeshes::make(DiagnosticSink& diag_sink) -> Status<CoreMeshes> {
	auto frame = diag_sink.make_frame(StringContext{[] { return "While making CoreMeshes:"; }});

	CoreMeshes meshes;
	try_init_mesh(meshes.ndc_quad, GlPrimitive::Triangles, ndc_quad_data, diag_sink);

	if (frame.has_errors()) {
		FR_LOG_ERROR("While initializing core meshes: {} error(s), {} warning(s)",
			frame.error_count(), frame.warning_count());
		return from_error;
	}
	if (frame.has_warnings()) {
		FR_LOG_WARN("While initializing core meshes: {} error(s), {} warning(s)",
			frame.error_count(), frame.warning_count());
	}
	return meshes;
}

auto Offscreen::try_init(glm::ivec2 dimensions, DiagnosticSink& diag_sink) -> Status<> {
	if (_color_texture.dimensions() == dimensions)
		return {};

	auto fb = GlFramebuffer::make(diag_sink);
	if (!fb)
		return from_error;

	auto depth_stencil = GlRenderbuffer::make_with_storage(
		GlRenderbufferFormat::Depth24Stencil8,
		dimensions,
		diag_sink
	);
	if (!depth_stencil)
		return from_error;


	auto frm = diag_sink.make_frame(StringContext{[] {
		return "While creating texture for offscreen framebuffer";
	}});

	auto color_tex = GlTexture2d::make_from_raw_data(
		GlTextureParams{
			.format = GlTextureFormat::RGB,
			.min_filtering = GlTextureMinFiltering::Nearest,
			.mag_filtering = GlTextureMagFiltering::Nearest,
			.num_mipmap_levels = 1,
			.autogen_mipmaps = false,
		},
		std::span<const std::byte>{},
		dimensions,
		GlPixelFormat::RGB,
		GlPixelDataType::UByte,
		diag_sink
	);
	if (!color_tex)
		return from_error;

	fb->bind();
	fb->attach(GlAttachmentPoint::color(0), *color_tex, diag_sink);
	fb->attach(GlAttachmentPoint::depth_stencil(), *depth_stencil, diag_sink);
	fb->complete(diag_sink);
	fb->unbind();

	if (frm.has_errors())
		return from_error;

	_depth_stencil_buffer = *std::move(depth_stencil);
	_color_texture = *std::move(color_tex);
	_framebuffer = *std::move(fb);

	return {};
}

struct CoreGraphicsInitSystem {
	static
	auto run(Runtime& runtime, const SdlData& sdl, DiagnosticSink& diag_sink) -> ErrorOr<> {
		auto offscreen = Offscreen{};
		if (auto res = offscreen.try_init(sdl.framebuffer_size, diag_sink); !res)
			return make_error(Errc::OpenGlError, "Failed to create offscreen buffer");
		FR_LOG_INFO_MSG("CoreGraphicsInitSystem: Created offscreen buffer");
		runtime.add_part(std::move(offscreen));

		auto screen_quad_shader = ScreenQuadShader::make(diag_sink);
		if (!screen_quad_shader) {
			return make_error(Errc::OpenGlError,
				"CoreGraphicsInitSystem: Failed to create ScreenQuadShader");
		}
		runtime.add_part(*std::move(screen_quad_shader));

		auto meshes = CoreMeshes::make(diag_sink);
		if (!meshes)
			return make_error(Errc::ResourceLoadingError, "CoreGraphicsInitSystem: Failed to "
				"create core meshes");
		runtime.add_part(*std::move(meshes));

		return {};
	}
};

struct ResizeOffscreenSystem {
	static
	auto run(
		Offscreen& offscreen,
		MessageReader<WindowResized>& messages,
		DiagnosticSink& diag_sink
	) -> ErrorOr<> {
		auto result = ErrorOr<>{};
		messages.for_last([&](const WindowResized& msg) {
			if (auto tmp = offscreen.try_init(msg.framebuffer_size, diag_sink); !tmp) {
				result = make_error(Errc::OpenGlError, "Failed to create offscreen buffer");
			}
		});
		return result;
	}
};

struct BindOffscreenSystem {
	static
	void run(Offscreen& offscreen) { offscreen.framebuffer().bind(); }
};

struct RenderScreenQuadSystem {
	static
	void run(Offscreen& offscreen, ScreenQuadShader& shader, CoreMeshes& meshes) {
		GlFramebuffer::bind_default();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		shader.use();
		shader.bind_texture(offscreen.color_texture());
		shader.draw(meshes.ndc_quad);
	}
};

void CoreGraphicsPreset::build(Runtime& runtime) {
	runtime
		.add_system<SetupPhase, CoreGraphicsInitSystem>()
		.add_system<LoopRenderEarlyPhase, ResizeOffscreenSystem>()
		.add_system<LoopRenderEarlyPhase, BindOffscreenSystem>()
		.add_system<FrameRenderEarlyPhase, RenderScreenQuadSystem>()
	;
}

} // namespace fr
