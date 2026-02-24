#include "asteroids/resources.hpp"

#include <cmrc/cmrc.hpp>

#include "fractal_box/core/logging.hpp"
#include "fractal_box/graphics/mesh_utils.hpp"
#include "fractal_box/resources/image_io.hpp"

CMRC_DECLARE(aster);

namespace aster {

namespace attribs {

[[maybe_unused]] static constexpr auto dummy = fr::GlAttribLabel::make<glm::vec2>("dummy");
[[maybe_unused]] static constexpr auto dummy2 = fr::GlAttribLabel::make<glm::vec2>("dummy", true);

} // namespace attribs

template<class Target, class Source>
static
void try_assign_with(Target& target, Source&& source) {
	if (source)
		target = *std::forward<Source>(source);
}

auto GameResources::init() -> fr::ErrorOr<> {
	// TODO: Check for OpenGL errors

	fr::DiagnosticStore errors;
	fr::DiagnosticStore warnings;

	// Shaders
	try_assign_with(this->color_shader, ColorShader::make(errors, warnings));
	try_assign_with(this->sprite_shader, SpriteShader::make(errors, warnings));

	// Meshes
	try_init_mesh(this->mesh_line, fr::GlPrimitive::LineStrip, fr::mesh_line_data, errors);
	try_init_mesh(this->mesh_square_solid, fr::GlPrimitive::Triangles, fr::mesh_square_solid_data,
		errors);
	try_init_mesh(this->mesh_square_wire, fr::GlPrimitive::LineLoop, fr::mesh_square_wire_data,
		errors);
	try_init_mesh(this->mesh_circle_solid, fr::GlPrimitive::TriangleFan,
		fr::make_circle_solid_mesh_data(0.5f, 32), errors);
	try_init_mesh(this->mesh_circle_wire, fr::GlPrimitive::LineLoop,
		fr::make_circle_wire_mesh_data(0.5f, 32), errors);

	// Textures
	const auto fs = cmrc::aster::get_filesystem();
	const auto params = fr::GlTextureParams{.autogen_mipmaps = false};
	try_init_texture(this->tex_spaceship, fs, "textures/spaceship.png", params, errors);
	try_init_texture(this->tex_bullet, fs, "textures/bullet.png", params, errors);
	try_init_texture(this->tex_asteroids[0], fs, "textures/asteroid_00.png", params, errors);
	try_init_texture(this->tex_asteroids[1], fs, "textures/asteroid_01.png", params, errors);
	try_init_texture(this->tex_asteroids[2], fs, "textures/asteroid_02.png", params, errors);
	try_init_texture(this->tex_asteroids[3], fs, "textures/asteroid_03.png", params, errors);
	try_init_texture(this->tex_plume, fs, "textures/plume.png", params, errors);

	if (const auto error_flags = fr::get_all_gl_error_flags()) {
		errors.push(fmt::format("Failed to initialize resources: OpenGL error {}",
			error_flags));
	}

#if FR_LOG_ERROR_ENABLED
	for (const auto& error : errors.get_all())
		FR_LOG_ERROR("{}", error);
#endif
#if FR_LOG_WARN_ENABLED
	for (const auto& warning : warnings.get_all())
		FR_LOG_WARN("{}", warning);
#endif

	if (!errors.empty()) {
		return make_error_fmt(fr::Errc::ResourceLoadingError, "While initializing resources: "
			"{} error(s), {} warning(s)", errors.size(), warnings.size());
	}
	else if (!warnings.empty()) {
		FR_LOG_WARN("While initializing resources: 0 error(s), {} warning(s)",
			errors.size(), warnings.size());
	}
	return {};
}

} // namespace aster
