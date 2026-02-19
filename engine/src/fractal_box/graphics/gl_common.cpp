#include "fractal_box/graphics/gl_common.hpp"

namespace fr {

auto GlState::capture_current() noexcept -> GlState {
	return {
		.programId = static_cast<GLuint>(get_current_gl_state_integer(GL_CURRENT_PROGRAM)),
		.textureUnit = static_cast<GLuint>(get_current_gl_state_integer(GL_ACTIVE_TEXTURE)),
		.textureId = static_cast<GLuint>(get_current_gl_state_integer(GL_TEXTURE_BINDING_2D)),
		.vertexArrayId = static_cast<GLuint>(get_current_gl_state_integer(GL_VERTEX_ARRAY_BINDING)),
		.arrayBufferId = static_cast<GLuint>(get_current_gl_state_integer(GL_ARRAY_BUFFER_BINDING)),
		.elementArrayBufferId = static_cast<GLuint>(
			get_current_gl_state_integer(GL_ELEMENT_ARRAY_BUFFER_BINDING)),
		.drawFramebufferId = static_cast<GLuint>(
			get_current_gl_state_integer(GL_DRAW_FRAMEBUFFER_BINDING)),
		.readFramebufferId = static_cast<GLuint>(
			get_current_gl_state_integer(GL_READ_FRAMEBUFFER_BINDING)),
	};
}

GlStateGuard::GlStateGuard() noexcept
	: _prev{GlState::capture_current()}
{ }

GlStateGuard::~GlStateGuard() {
	glUseProgram(_prev.programId);
	glActiveTexture(_prev.textureUnit);
	glBindTexture(GL_TEXTURE_2D, _prev.textureId);
	glBindVertexArray(_prev.vertexArrayId);
	glBindBuffer(GL_ARRAY_BUFFER, _prev.arrayBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _prev.elementArrayBufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _prev.drawFramebufferId);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _prev.readFramebufferId);
}

} // namespace fr
