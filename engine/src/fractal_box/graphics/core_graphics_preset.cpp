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

auto CoreMeshes::make() -> ErrorOr<CoreMeshes> {
	DiagnosticStore error_sink;
	DiagnosticStore warning_sink;

	CoreMeshes meshes;
	try_init_mesh(meshes.ndc_quad, GlPrimitive::Triangles, ndc_quad_data, error_sink);
	if (!error_sink.empty()) {
		return make_error_fmt(Errc::ResourceLoadingError, "While initializing core meshes: "
			"{} error(s), {} warning(s)", error_sink.size(), warning_sink.size());
	}
	return ErrorOr<CoreMeshes>{std::in_place, std::move(meshes)};
}

auto Offscreen::try_init(glm::ivec2 dimensions, IDiagnosticSink& errors) -> ErrorOr<> {
	if (_color_texture.dimensions() == dimensions)
		return {};

	auto fb = GlFramebuffer::make();
	if (!fb)
		return extract_unexpected(std::move(fb));
	auto depth_stencil = GlRenderbuffer::make_with_storage(
		GlRenderbufferFormat::Depth24Stencil8,
		dimensions
	);
	if (!depth_stencil)
		return extract_unexpected(std::move(depth_stencil));

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
		errors
	);
	if (!color_tex)
		return make_error(Errc::OpenGlError,
			"CoreGraphicsInitSystem: Failed to create texture for ofscreen framebuffer");
	fb->bind();
	fb->attach(GlAttachmentPoint::color(0), *color_tex);
	fb->attach(GlAttachmentPoint::depth_stencil(), *depth_stencil);
	fb->complete();
	fb->unbind();

	_depth_stencil_buffer = *std::move(depth_stencil);
	_color_texture = *std::move(color_tex);
	_framebuffer = *std::move(fb);

	return {};
}

struct CoreGraphicsInitSystem {
	static
	auto run(Runtime& runtime, const SdlData& sdl) -> ErrorOr<> {
		auto errors = DiagnosticStore{};
		auto warnings = DiagnosticStore{};

		auto offscreen = Offscreen{};
		if (auto res = offscreen.try_init(sdl.framebuffer_size, errors); !res)
			return res;
		FR_LOG_INFO_MSG("CoreGraphicsPreset: Created offscreen buffer");
		runtime.add_part(std::move(offscreen));

		auto screen_quad_shader = ScreenQuadShader::make(errors, warnings);
		if (!screen_quad_shader) {
			for (auto e : errors.get_all())
				FR_LOG_ERROR("{}", e);
			return make_error(Errc::OpenGlError,
				"CoreGraphicsInitSystem: Failed to create ScreenQuadShader");
		}
		runtime.add_part(*std::move(screen_quad_shader));

		auto meshes = CoreMeshes::make();
		if (!meshes)
			return extract_unexpected(std::move(meshes));
		runtime.add_part(*std::move(meshes));

		return {};
	}
};

struct ResizeOffscreenSystem {
	static
	auto run(Offscreen& offscreen, MessageReader<WindowResized>& messages) -> ErrorOr<> {
		auto errors = DiagnosticStore{};
		auto result = ErrorOr<>{};
		messages.for_last([&](const WindowResized& msg) {
			if (auto tmp = offscreen.try_init(msg.framebuffer_size, errors); !tmp) {
				result = std::move(tmp);
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
