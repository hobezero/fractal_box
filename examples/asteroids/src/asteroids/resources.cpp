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

auto GameResources::init(fr::DiagnosticSink& diag_sink) -> fr::ErrorOr<> {
	auto obs = diag_sink.make_observer();

	// Shaders
	try_assign_with(this->color_shader, ColorShader::make(diag_sink));
	try_assign_with(this->sprite_shader, SpriteShader::make(diag_sink));

	// Meshes
	try_init_mesh(this->mesh_line, fr::GlPrimitive::LineStrip, fr::mesh_line_data, diag_sink);
	try_init_mesh(this->mesh_square_solid, fr::GlPrimitive::Triangles, fr::mesh_square_solid_data,
		diag_sink);
	try_init_mesh(this->mesh_square_wire, fr::GlPrimitive::LineLoop, fr::mesh_square_wire_data,
		diag_sink);
	try_init_mesh(this->mesh_circle_solid, fr::GlPrimitive::TriangleFan,
		fr::make_circle_solid_mesh_data(0.5f, 32), diag_sink);
	try_init_mesh(this->mesh_circle_wire, fr::GlPrimitive::LineLoop,
		fr::make_circle_wire_mesh_data(0.5f, 32), diag_sink);

	// Textures
	const auto fs = cmrc::aster::get_filesystem();
	const auto params = fr::GlTextureParams{.autogen_mipmaps = false};
	try_init_texture(this->tex_spaceship, fs, "textures/spaceship.png", params, diag_sink);
	try_init_texture(this->tex_bullet, fs, "textures/bullet.png", params, diag_sink);
	try_init_texture(this->tex_asteroids[0], fs, "textures/asteroid_00.png", params, diag_sink);
	try_init_texture(this->tex_asteroids[1], fs, "textures/asteroid_01.png", params, diag_sink);
	try_init_texture(this->tex_asteroids[2], fs, "textures/asteroid_02.png", params, diag_sink);
	try_init_texture(this->tex_asteroids[3], fs, "textures/asteroid_03.png", params, diag_sink);
	try_init_texture(this->tex_plume, fs, "textures/plume.png", params, diag_sink);

	if (const auto error_flags = fr::get_all_gl_error_flags()) {
		diag_sink(fr::StringError{[error_flags] {
			return fmt::format("Failed to initialize resources: OpenGL error {}", error_flags);
		}});
	}

	if (obs.has_errors()) {
		FR_LOG_ERROR("While initializing core meshes: {} error(s), {} warning(s)",
			obs.error_count(), obs.warning_count());
		return fr::make_error(fr::Errc::ResourceLoadingError, "Failed to load game resources");
	}
	else if (obs.has_warnings()) {
		FR_LOG_WARN("While initializing core meshes: {} error(s), {} warning(s)",
			obs.error_count(), obs.warning_count());
	}
	return {};
}

} // namespace aster
