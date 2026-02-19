#ifndef FRACTAL_BOX_GRAPHICS_SHADER_UTILS_HPP
#define FRACTAL_BOX_GRAPHICS_SHADER_UTILS_HPP

#include "fractal_box/graphics/gl_shader_program.hpp"
#include "fractal_box/resources/resource_utils.hpp"

namespace fr {

struct MakeLinkedShaderProgramParams {
	NonDefault<std::string> name;
	NonDefault<GlVersion> version;
	cmrc::embedded_filesystem filesystem;
	const std::string& vertexShaderFilePath;
	NonDefault<std::string> vertexShaderName;
	const std::string& fragmentShaderFilePath;
	NonDefault<std::string> fragmentShaderName;
	IDiagnosticSink& error_sink;
	IDiagnosticSink& warning_sink;
};

/// @brief Helper function to create a GlShaderProgram from shader sources defined in the
/// embedded resource files
/// @note Implemented only for two shader stages as the most common use case
/// (vertex + fragment shaders). Not implemented as a variadic template to avoid std::tuple madness
inline
auto make_linked_shader_program_from_resources(
	MakeLinkedShaderProgramParams&& params
) -> std::optional<GlShaderProgram> {
	auto vertex_text = try_get_resource_string(
		params.filesystem,
		params.vertexShaderFilePath,
		params.error_sink
	);
	auto fragment_text = try_get_resource_string(
		params.filesystem,
		params.fragmentShaderFilePath,
		params.error_sink
	);
	if (!vertex_text || !fragment_text)
		return std::nullopt;

	return GlShaderProgram::makeLinked(
		std::move(*params.name),
		to_span<GlShader::Params>({
			{
				.type = GlShaderType::Vertex,
				.version = *params.version,
				.sources = std::vector{*std::move(vertex_text)},
				.name = std::move(params.vertexShaderName)
			},
			{
				.type = GlShaderType::Fragment,
				.version = *params.version,
				.sources = std::vector{*std::move(fragment_text)},
				.name = std::move(*params.fragmentShaderName)
			}
		}),
		params.error_sink, params.warning_sink
	);
}

} // namespace fr
#endif
