#ifndef ASTEROIDS_SHADERS_HPP
#define ASTEROIDS_SHADERS_HPP

/// @file
/// @brief GlShader classes used by the game

#include "fractal_box/core/color.hpp"
#include "fractal_box/graphics/gl_shader_program.hpp"
#include "fractal_box/graphics/gl_texture.hpp"
#include "fractal_box/graphics/mesh_utils.hpp"

namespace aster {

/// @brief GlShader to render single-colored 2D objects
class ColorShader final: public fr::GlShaderProgram {
	using Base = GlShaderProgram;

	struct Uniforms {
		fr::GlUniform<glm::mat3> viewProjMat;
		fr::GlUniform<GLfloat> depth;
		fr::GlUniform<glm::vec4> color;
	};

public:
	static constexpr fr::GlShaderAttrib attributes[] = {
		{fr::attrib_pos, 0},
		{fr::attrib_tex_coords, 1},
	};

	static auto make(
		fr::IDiagnosticSink& error_sink, fr::IDiagnosticSink& warning_sink
	) -> std::optional<ColorShader>;

	explicit ColorShader() = default;

	void setViewProjMatUniform(const glm::mat3& value) noexcept {
		set_uniform(_uniforms.viewProjMat, value);
	}

	void setDepthUniform(decltype(Uniforms::depth)::type value) noexcept {
		set_uniform(_uniforms.depth, value);
	}

	void setColorUniform(fr::Color4 value) noexcept {
		set_uniform(_uniforms.color, value);
	}

private:
	ColorShader(fr::AdoptInit, Base&& base, Uniforms&& uniforms) noexcept;

private:
	Uniforms _uniforms {fr::uninitialized, fr::uninitialized, fr::uninitialized};
};

/// @brief GlShader to render textured 2D objects
class SpriteShader final: public fr::GlShaderProgram {
	using Base = fr::GlShaderProgram;

	struct Uniforms {
		fr::GlUniform<glm::mat3> viewProjMat;
		fr::GlUniform<GLfloat> depth;
	};

public:
	static constexpr fr::GlShaderAttrib attributes[] = {
		{fr::attrib_pos, 0},
		{fr::attrib_tex_coords, 1},
	};

	static auto make(
		fr::IDiagnosticSink& error_sink, fr::IDiagnosticSink& warning_sink
	) -> std::optional<SpriteShader>;

	explicit SpriteShader() = default;

	void setViewProjMatUniform(const glm::mat3& value) noexcept {
		set_uniform(_uniforms.viewProjMat, value);
	}

	void setDepthUniform(decltype(Uniforms::depth)::type value) noexcept {
		set_uniform(_uniforms.depth, value);
	}

	void bindTexture(const fr::GlTexture2d& texture) noexcept;

private:
	SpriteShader(fr::AdoptInit, Base&& base, Uniforms&& uniforms) noexcept;

private:
	Uniforms _uniforms {fr::uninitialized, fr::uninitialized};
};

} // namespace aster
#endif
