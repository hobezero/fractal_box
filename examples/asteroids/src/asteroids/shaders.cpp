#include "asteroids/shaders.hpp"

#include <fmt/format.h>

#include "fractal_box/graphics/gl_common.hpp"
#include "fractal_box/graphics/shader_utils.hpp"

CMRC_DECLARE(aster);

namespace aster {

#if FR_TARGET_GLES3_EMULATED
static constexpr auto gl_version = fr::GlVersion::GlEs300;
#else
static constexpr auto gl_version = fr::GlVersion::Gl330;
#endif

auto ColorShader::make(fr::DiagnosticSink& diag_sink) -> fr::Status<ColorShader> {
	auto program = fr::make_linked_shader_program_from_resources({
		.name = "ColorShader",
		.version = gl_version,
		.filesystem = cmrc::aster::get_filesystem(),
		.vertex_shader_file_path = "shaders/color.vert",
		.vertex_shader_name = "color.vert",
		.fragment_shader_file_path = "shaders/color.frag",
		.fragment_shader_name = "color.frag",
		.diag_sink = diag_sink,
	});
	if (!program)
		return fr::from_error;

	auto u_pack = make_gl_uniforms_object(*program, [](auto unwrap) {
		return Uniforms {
			.view_proj_mat = unwrap("u_view_proj_mat"),
			.depth = unwrap("u_depth"),
			.color = unwrap("u_color"),
		};
	}, diag_sink);
	if (!u_pack)
		return fr::from_error;

	return ColorShader{fr::adopt, std::move(*program), std::move(*u_pack)};
}

ColorShader::ColorShader(fr::AdoptInit, Base&& base, Uniforms&& uniforms) noexcept:
	Base{std::move(base)},
	_uniforms{std::move(uniforms)}
{ }

void SpriteShader::bind_texture(const fr::GlTexture2d& texture) noexcept {
	glActiveTexture(GL_TEXTURE0);
	texture.bind();
}

auto SpriteShader::make(fr::DiagnosticSink& diag_sink) -> fr::Status<SpriteShader> {
	auto program = fr::make_linked_shader_program_from_resources({
		.name = "SpriteShader",
		.version = gl_version,
		.filesystem = cmrc::aster::get_filesystem(),
		.vertex_shader_file_path = "shaders/sprite.vert",
		.vertex_shader_name = "sprite.vert",
		.fragment_shader_file_path = "shaders/sprite.frag",
		.fragment_shader_name = "sprite.frag",
		.diag_sink = diag_sink,
	});
	if (!program)
		return fr::from_error;

	auto u_pack = make_gl_uniforms_object(*program, [](auto unwrap) {
		return Uniforms {
			.view_proj_mat = unwrap("u_view_proj_mat"),
			.depth = unwrap("u_depth")
		};
	}, diag_sink);
	if (!u_pack)
		return fr::from_error;

	return SpriteShader{fr::adopt, std::move(*program), std::move(*u_pack)};
}

SpriteShader::SpriteShader(fr::AdoptInit, Base&& base, Uniforms&& uniforms) noexcept:
	Base{std::move(base)},
	_uniforms{std::move(uniforms)}
{ }

} // namespace aster
