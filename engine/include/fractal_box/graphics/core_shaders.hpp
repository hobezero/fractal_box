#ifndef FRACTAL_BOX_GRAPHICS_CORE_SHADERS_HPP
#define FRACTAL_BOX_GRAPHICS_CORE_SHADERS_HPP

#include <optional>

#include "fractal_box/graphics/gl_shader_program.hpp"
#include "fractal_box/graphics/gl_texture.hpp"

namespace fr {

class ScreenQuadShader final: public GlShaderProgram {
	using Base = GlShaderProgram;

public:
	static
	auto make(
		IDiagnosticSink& error_sink, IDiagnosticSink& warning_sink
	) -> std::optional<ScreenQuadShader>;

	explicit
	ScreenQuadShader() = default;

	void bind_texture(const GlTexture2d& texture) noexcept;

private:
	ScreenQuadShader(AdoptInit, Base&& base) noexcept;
};

} // namespace fr
#endif
