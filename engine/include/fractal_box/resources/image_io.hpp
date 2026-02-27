#ifndef FRACTAL_BOX_RESOURCES_IMAGE_IO_HPP
#define FRACTAL_BOX_RESOURCES_IMAGE_IO_HPP

#include <optional>
#include <string>

#include <cmrc/cmrc.hpp>

#include "fractal_box/core/error_handling/diagnostic.hpp"
#include "fractal_box/graphics/gl_texture.hpp"

namespace fr {

auto make_texture2d_from_resources(
	cmrc::embedded_filesystem assets_fs,
	const std::string& file_name,
	const GlTextureParams& texture_params,
	IDiagnosticSink& error_sink
) -> std::optional<GlTexture2d>;

inline
void try_init_texture(
	GlTexture2d& target_texture,
	cmrc::embedded_filesystem fs,
	const std::string& file_name,
	const GlTextureParams& params,
	IDiagnosticSink& error_sink
) {
	FR_GL_ASSERT_FAST();
	auto texture = make_texture2d_from_resources(fs, file_name,
		params, error_sink);
	if (texture)
		target_texture = std::move(*texture);
}

} // namespace fr
#endif // include guard
