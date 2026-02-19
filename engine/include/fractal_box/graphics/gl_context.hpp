#ifndef FRACTAL_BOX_GRAPHICS_GL_CONTEXT_HPP
#define FRACTAL_BOX_GRAPHICS_GL_CONTEXT_HPP

#include <glad/glad.h>

#include <glm/vec3.hpp>

namespace fr {

struct OglState {

	GLuint program_id = 0;
	GLuint texture_unit = 0;
	GLuint texture_id = 0;
	GLuint vertex_array_id = 0;
	GLuint array_buffer_id = 0;
	GLuint element_array_buffer_id = 0;
	GLuint draw_framebuffer_id = 0;
	GLuint read_framebuffer_id = 0;
	glm::vec3 clear_color = {};
};

class GlContext {
public:
	GlContext(const GlContext&) = delete;
	auto operator=(const GlContext&) -> GlContext& = delete;

	GlContext(GlContext&& other) noexcept;
	auto operator=(GlContext&& other) noexcept -> GlContext&;

	static
	auto make_from_current() -> GlContext;

private:
	GlContext();
	OglState _state {};
};

} // namespace fr
#endif // include guard
