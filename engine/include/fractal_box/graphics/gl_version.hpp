#ifndef FRACTAL_BOX_GRAPHICS_GL_VERSION_HPP
#define FRACTAL_BOX_GRAPHICS_GL_VERSION_HPP

#include <string_view>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/platform.hpp"

// Supported targets: OpenGL 3.3+, OpenGL ES 3.0+, WebGL 2.0+
// FIXME: Proper GLES detection

#if FR_COMP_EMSCRIPTEN
#	define FR_TARGET_GL_DESKTOP 0
#	define FR_TARGET_WEBGL2 1
	/// Either OpenGL ES 3.0 or WebGL 2
#	define FR_TARGET_GLES3_EMULATED 1
#	define FR_TARGET_GLES3 0
#else
#	define FR_TARGET_GL_DESKTOP 1
#	define FR_TARGET_WEBGL2 0
#	define FR_TARGET_GLES3_EMULATED 0
#	define FR_TARGET_GLES3 0
#endif

namespace fr {

enum class GraphicsApiKind {
	Unspecified,
#if FR_TARGET_GL_DESKTOP
	OpenGl,
#endif
	OpenGlEs,
};

enum class GlVersion {
	Unspecified,
#if FR_TARGET_GL_DESKTOP
	Gl210 [[deprecated("OpenGL 2.1 is not explicitly supported")]],
	Gl300 [[deprecated("OpenGL 3.0 is not explicitly supported")]],
	Gl310 [[deprecated("OpenGL 3.1 is not explicitly supported")]],
	Gl320 [[deprecated("OpenGL 3.2 is not explicitly supported")]],
	Gl330,
	Gl400,
	Gl410,
	Gl420,
	Gl430,
	Gl440,
	Gl450,
	Gl460,
#endif
	GlEs200 [[deprecated("OpenGL ES 2.0 is not explicitly supported")]],
	GlEs300,
#if !FR_TARGET_WEBGL2
	GlEs310,
	GlEs320,
#endif
};

FR_DIAGNOSTIC_PUSH
HEDLEY_DIAGNOSTIC_DISABLE_DEPRECATED

class GlVersionPair {
	static constexpr auto null_version = static_cast<GlVersion>(
		static_cast<std::underlying_type_t<GlVersion>>(-1));

public:
	explicit constexpr
	GlVersionPair() noexcept:
		_api{GraphicsApiKind::Unspecified},
		_major{0},
		_minor{0}
	{ }

	explicit constexpr
	GlVersionPair(GraphicsApiKind api, int major, int minor) noexcept:
		_api{api},
		_major{major},
		_minor{minor}
	{
		FR_ASSERT_MSG(combine() != null_version, "Invalid version");
	}

	explicit constexpr
	GlVersionPair(GlVersion version) noexcept {
		using enum GlVersion;
		switch (version) {
			case Unspecified: set(GraphicsApiKind::Unspecified, 0, 0); return;
	#if FR_TARGET_GL_DESKTOP
			case Gl210: set_unchecked(GraphicsApiKind::OpenGl, 2, 1); return;
			case Gl300: set_unchecked(GraphicsApiKind::OpenGl, 3, 0); return;
			case Gl310: set_unchecked(GraphicsApiKind::OpenGl, 3, 1); return;
			case Gl320: set_unchecked(GraphicsApiKind::OpenGl, 3, 2); return;
			case Gl330: set_unchecked(GraphicsApiKind::OpenGl, 3, 3); return;
			case Gl400: set_unchecked(GraphicsApiKind::OpenGl, 4, 0); return;
			case Gl410: set_unchecked(GraphicsApiKind::OpenGl, 4, 1); return;
			case Gl420: set_unchecked(GraphicsApiKind::OpenGl, 4, 2); return;
			case Gl430: set_unchecked(GraphicsApiKind::OpenGl, 4, 3); return;
			case Gl440: set_unchecked(GraphicsApiKind::OpenGl, 4, 4); return;
			case Gl450: set_unchecked(GraphicsApiKind::OpenGl, 4, 5); return;
			case Gl460: set_unchecked(GraphicsApiKind::OpenGl, 4, 6); return;
	#endif
			case GlEs200: set_unchecked(GraphicsApiKind::OpenGlEs, 2, 0); return;
			case GlEs300: set_unchecked(GraphicsApiKind::OpenGlEs, 3, 0); return;
	#if !FR_TARGET_WEBGL2
			case GlEs310: set_unchecked(GraphicsApiKind::OpenGlEs, 3, 1); return;
			case GlEs320: set_unchecked(GraphicsApiKind::OpenGlEs, 3, 2); return;
	#endif
		}
		FR_UNREACHABLE_MSG("Unknown OpenGL version");
	}

	constexpr
	void set(GraphicsApiKind new_api, int new_major, int new_minor) noexcept {
		set_unchecked(new_api, new_major, new_minor);
		FR_ASSERT_MSG(combine() != null_version, "Invalid version");
	}

	constexpr
	auto combine() const noexcept -> GlVersion {
		using enum GlVersion;
		switch (_api) {
			case GraphicsApiKind::Unspecified:
				return Unspecified;
#if FR_TARGET_GL_DESKTOP
			case GraphicsApiKind::OpenGl:
				if (_major == 2 && _minor == 1) return Gl210;
				if (_major == 3 && _minor == 0) return Gl300;
				if (_major == 3 && _minor == 1) return Gl310;
				if (_major == 3 && _minor == 2) return Gl320;
				if (_major == 3 && _minor == 3) return Gl330;
				if (_major == 4 && _minor == 0) return Gl400;
				if (_major == 4 && _minor == 1) return Gl410;
				if (_major == 4 && _minor == 2) return Gl420;
				if (_major == 4 && _minor == 3) return Gl430;
				if (_major == 4 && _minor == 4) return Gl440;
				if (_major == 4 && _minor == 5) return Gl450;
				if (_major == 4 && _minor == 6) return Gl460;
				break;
#endif
			case GraphicsApiKind::OpenGlEs:
				if (_major == 2 && _minor == 0) return GlEs200;
				if (_major == 3 && _minor == 0) return GlEs300;
#if !FR_TARGET_WEBGL2
				if (_major == 3 && _minor == 1) return GlEs310;
				if (_major == 3 && _minor == 2) return GlEs320;
#endif
				break;
		}
		return null_version;
	}

	constexpr
	auto api() const noexcept -> GraphicsApiKind { return _api; }

	constexpr
	auto major() const noexcept -> int { return _major; }

	constexpr
	auto minor() const noexcept -> int { return _minor; }

private:
	constexpr
	void set_unchecked(GraphicsApiKind new_api, int new_major, int new_minor) noexcept {
		_api = new_api;
		_major = new_major;
		_minor = new_minor;
	}

private:
	GraphicsApiKind _api = GraphicsApiKind::Unspecified;
	int _major = 0;
	int _minor = 0;
};

inline constexpr
auto gl_shader_version_line(GlVersion version) -> std::string_view {
	using enum GlVersion;
	switch (version) {
		case Unspecified: return {};
#if FR_TARGET_GL_DESKTOP
		case Gl210: return "#version 120\n";
		case Gl300: return "#version 130\n";
		case Gl310: return "#version 140\n";
		case Gl320: return "#version 150\n";
		case Gl330: return "#version 330\n";
		case Gl400: return "#version 400\n";
		case Gl410: return "#version 410\n";
		case Gl420: return "#version 420\n";
		case Gl430: return "#version 430\n";
		case Gl440: return "#version 440\n";
		case Gl450: return "#version 450\n";
		case Gl460: return "#version 460\n";
#endif
		case GlEs200: return "#version 100\n";
		case GlEs300: return "#version 300 es\n";
#if !FR_TARGET_WEBGL2
		case GlEs310: return "#version 310 es\n";
		case GlEs320: return "#version 320 es\n";
#endif
	}
	FR_UNREACHABLE_MSG("Unknown OpenGL version");
}

FR_DIAGNOSTIC_POP

} // namespace fr
#endif // include guard
