#include "fractal_box/graphics/core_shaders.hpp"

#include "fractal_box/graphics/shader_utils.hpp"

CMRC_DECLARE(fr);

namespace fr {

#if FR_TARGET_GLES3_EMULATED
static constexpr auto gl_version = GlVersion::GlEs300;
#else
static constexpr auto gl_version = GlVersion::Gl330;
#endif

auto ScreenQuadShader::make(
	IDiagnosticSink& error_sink, IDiagnosticSink& warning_sink
) -> std::optional<ScreenQuadShader> {
	auto program = make_linked_shader_program_from_resources({
		.name = "ScreenQuadShader",
		.version = gl_version,
		.filesystem = cmrc::fr::get_filesystem(),
		.vertexShaderFilePath = "shaders/screen_quad.vert.glsl",
		.vertexShaderName = "screen_quad.vert.glsl",
		.fragmentShaderFilePath = "shaders/screen_quad.frag.glsl",
		.fragmentShaderName = "screen_quad.frag.glsl",
		.error_sink = error_sink,
		.warning_sink = warning_sink
	});
	if (!program)
		return std::nullopt;

	return ScreenQuadShader{adopt, *std::move(program)};
}

ScreenQuadShader::ScreenQuadShader(AdoptInit, Base&& base) noexcept:
	Base{std::move(base)}
{ }

void ScreenQuadShader::bind_texture(const GlTexture2d& texture) noexcept {
	glActiveTexture(GL_TEXTURE0);
	texture.bind();
}

} // namespace fr
