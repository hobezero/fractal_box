#ifndef FRACTAL_BOX_GRAPHICS_GL_COMMON_HPP
#define FRACTAL_BOX_GRAPHICS_GL_COMMON_HPP

#include <fmt/format.h>
#include <glad/glad.h>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/graphics/gl_version.hpp"

namespace fr {

using GlObjectId = GLuint;
using GlUniformLocation = GLint;
using GlAttribLocation = GLint;

class GlObject {
public:
	GlObject(const GlObject&) = delete;
	GlObject& operator=(const GlObject&) = delete;

protected:
	GlObject() = default;

	GlObject(GlObject&&) = default;
	GlObject& operator=(GlObject&&) = default;

	~GlObject() = default;
};

/// @brief Type-safe error code returned from `glGetError()`. Stored in a bitflag-friendly manner
/// (i.e. each value is a power of two)
enum class GlErrorFlag: uint8_t {
	Unknown = 1u << 0,
	InvalidEnum = 1u << 1,
	InvalidValue = 1u << 2,
	InvalidOperation = 1u << 3,
	InvalidFramebufferOperation = 1u << 4,
	OutOfMemory = 1u << 5,
	StackUnderflow = 1u << 6,
	StackOverflow = 1u << 7,
};

using GlErrorFlags = Flags<GlErrorFlag>;

inline
auto get_all_gl_error_flags() noexcept -> GlErrorFlags {
	auto flags = GlErrorFlags{};
	for (GLenum err_code; (err_code = glGetError()) != GL_NO_ERROR;) {
		using enum GlErrorFlag;
		switch (err_code) {
			case GL_INVALID_ENUM: flags.set(InvalidEnum); break;
			case GL_INVALID_VALUE: flags.set(InvalidValue); break;
			case GL_INVALID_OPERATION: flags.set(InvalidOperation); break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: flags.set(InvalidFramebufferOperation); break;
			case GL_OUT_OF_MEMORY: flags.set(OutOfMemory); break;
			case GL_STACK_UNDERFLOW: flags.set(StackUnderflow); break;
			case GL_STACK_OVERFLOW: flags.set(StackOverflow); break;
			default: flags.set(Unknown); break;
		}
	}
	return flags;
}

#define FR_GL_CHECK_ASSERT() \
	do { \
		const auto error_flags = ::fr::get_all_gl_error_flags(); \
		FR_PANIC_CHECK_FMT(error_flags.empty(), "OpenGL error '{}'", error_flags); \
	} while (false)

#if FR_ASSERT_FAST_ENABLED
#	define FR_GL_ASSERT_FAST() FR_GL_CHECK_ASSERT()
#else
#	define FR_GL_ASSERT_FAST() static_cast<void>(0)
#endif

#if FR_ASSERT_ENABLED
#	define FR_GL_ASSERT() FR_GL_CHECK_ASSERT()
#else
#	define FR_GL_ASSERT() static_cast<void>(0)
#endif

#if FR_ASSERT_AUDIT_ENABLED
#	define FR_GL_ASSERT_AUDIT() FR_GL_CHECK_ASSERT()
#else
#	define FR_GL_ASSERT_AUDIT() static_cast<void>(0)
#endif

inline
auto get_current_gl_state_integer(GLenum pname) noexcept -> GLint {
	GLint value = 0;
	glGetIntegerv(pname, &value);
	return value;
}

struct GlState {
	[[nodiscard]] static
	auto capture_current() noexcept -> GlState;

public:
	GLuint programId = 0;
	GLuint textureUnit = 0;
	GLuint textureId = 0;
	GLuint vertexArrayId = 0;
	GLuint arrayBufferId = 0;
	GLuint elementArrayBufferId = 0;
	GLuint drawFramebufferId = 0;
	GLuint readFramebufferId = 0;
};

class GlStateGuard {
public:
	[[nodiscard]] explicit
	GlStateGuard() noexcept;

	GlStateGuard(const GlStateGuard&) = delete;
	auto operator=(const GlStateGuard&) -> GlStateGuard& = delete;

	GlStateGuard(GlStateGuard&&) = delete;
	auto operator=(GlStateGuard&&) -> GlStateGuard& = delete;

	~GlStateGuard();

private:
	GlState _prev;
};

} // namespace fr

template<>
struct fmt::formatter<fr::GlErrorFlags>: formatter<fmt::string_view> {
	auto format(fr::GlErrorFlags flags, auto& ctx) const {
		using Base = formatter<fmt::string_view>;
		auto is_first = true;
		for (const auto [flag, name] : table) {
			if (flags.test(flag)) {
				if (is_first)
					is_first = false;
				else
					Base::format("|", ctx);
				Base::format(name, ctx);
			}
		}
		return ctx.out();
	}

private:
	using enum fr::GlErrorFlag;
	struct Pair { fr::GlErrorFlag flag; fmt::string_view name; };
	static constexpr Pair table[] = {
		{ Unknown, "unknown_error" },
		{ InvalidEnum, "GL_INVALID_ENUM" },
		{ InvalidValue, "GL_INVALID_VALUE" },
		{ InvalidOperation, "GL_INVALID_OPERATION" },
		{ InvalidFramebufferOperation, "GL_INVALID_FRAMEBUFFER_OPERATION" },
		{ OutOfMemory, "GL_OUT_OF_MEMORY" },
		{ StackUnderflow, "GL_STACK_UNDERFLOW" },
		{ StackOverflow, "GL_STACK_OVERFLOW" },
	};
};

#endif // include guard
