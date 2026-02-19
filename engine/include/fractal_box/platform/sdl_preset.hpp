#ifndef FRACTAL_BOX_PLATFORM_SDL_PRESET_HPP
#define FRACTAL_BOX_PLATFORM_SDL_PRESET_HPP

#include <memory>
#include <string>
#include <string_view>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <glm/vec2.hpp>

#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/graphics/gl_version.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/runtime/message_traits.hpp"
#include "fractal_box/runtime/runtime.hpp"

namespace fr {

template<>
struct MessageTraits<SDL_Event> {
	using TickAt = FrameEndPhase;

	static constexpr auto default_ttl = MessageTtl::OneTick;
};

enum class SwapInterval {
	Immediate = 0,
	VSync = 1,
	AdaptiveVSync = -1,
};

inline constexpr
auto to_string_view(SwapInterval swap_interval) noexcept -> std::string_view {
	switch (swap_interval) {
		case SwapInterval::Immediate: return "Immediate";
		case SwapInterval::VSync: return "VSync";
		case SwapInterval::AdaptiveVSync: return "AdaptiveVSync";
	}
	FR_UNREACHABLE();
}

enum class SdlWindowFlag: std::underlying_type_t<SDL_WindowFlags> {
	Fullscreen = SDL_WINDOW_FULLSCREEN,
	OpenGl = SDL_WINDOW_OPENGL,
	Shown = SDL_WINDOW_SHOWN,
	Hidden = SDL_WINDOW_HIDDEN,
	Borderless = SDL_WINDOW_BORDERLESS,
	Resizable = SDL_WINDOW_RESIZABLE,
	Minimized = SDL_WINDOW_MINIMIZED,
	Maximized = SDL_WINDOW_MAXIMIZED,
	MouseGrabbed = SDL_WINDOW_MOUSE_GRABBED,
	InputFocus = SDL_WINDOW_INPUT_FOCUS,
	MouseFocus = SDL_WINDOW_MOUSE_FOCUS,
	FullscreenDesktop = SDL_WINDOW_FULLSCREEN_DESKTOP,
	Foreign = SDL_WINDOW_FOREIGN,
	AllowHighDpi = SDL_WINDOW_ALLOW_HIGHDPI,
	MouseCapture = SDL_WINDOW_MOUSE_CAPTURE,
	AlwaysOnTop = SDL_WINDOW_ALWAYS_ON_TOP,
	SkipTaskbar = SDL_WINDOW_SKIP_TASKBAR,
	Utility = SDL_WINDOW_UTILITY,
	Tooltip = SDL_WINDOW_TOOLTIP,
	PopupMenu = SDL_WINDOW_POPUP_MENU,
	KeyboardGrabbed = SDL_WINDOW_KEYBOARD_GRABBED,
	Vulkan = SDL_WINDOW_VULKAN,
	Metal = SDL_WINDOW_METAL,
	InputGrabbed = SDL_WINDOW_INPUT_GRABBED,
};

using SdlWindowFlags = Flags<SdlWindowFlag>;

inline constexpr auto default_sdl_window_flags = SdlWindowFlags{{SdlWindowFlag::Shown,
	SdlWindowFlag::OpenGl}};

struct SdlOptions {
	std::string window_title;
	glm::ivec2 window_size = {800, 800};
	SdlWindowFlags window_flags = default_sdl_window_flags;
	GlVersion gl_version = GlVersion::Gl430;
	bool with_gl_debug_output = true;
	bool with_doublebuffer = true;
	int depth_size_bits = 24;
	SwapInterval swap_interval = SwapInterval::Immediate;
};

using SdlWindowHandle = std::unique_ptr<SDL_Window, FuncLifter<&SDL_DestroyWindow>>;
using SdlGlContextHandle = std::unique_ptr<std::remove_pointer_t<SDL_GLContext>,
	FuncLifter<&SDL_GL_DeleteContext>>;

#if 0 // TODO: Implement
enum class SdlSubsystemFlag {
	Timer = SDL_INIT_TIMER,
	Audio = SDL_INIT_AUDIO,
	Video = SDL_INIT_VIDEO,
	Joystick = SDL_INIT_JOYSTICK,
	Haptic = SDL_INIT_HAPTIC,
	Gamecontroller = SDL_INIT_GAMECONTROLLER,
	Events = SDL_INIT_EVENTS,
	Sensor = SDL_INIT_SENSOR,
};
#endif

class SdlGuard {
public:
	SdlGuard() = default;

	static
	auto make(uint32_t init_flags) -> ErrorOr<SdlGuard>;

	SdlGuard(const SdlGuard&) = delete;
	auto operator=(const SdlGuard&) = delete;

	SdlGuard(SdlGuard&&) noexcept = default;
	auto operator=(SdlGuard&&) noexcept -> SdlGuard& = default;

	~SdlGuard();

	auto subsystem_flags() -> uint32_t { return _subsystem_flags.value(); }

private:
	explicit constexpr
	SdlGuard(AdoptInit, uint32_t init_flags) noexcept:
		_subsystem_flags{init_flags}
	{ }

private:
	WithDefault<uint32_t, 0> _subsystem_flags;
};

struct SdlData {
	SdlGuard guard;
	SdlWindowHandle window;
	SdlGlContextHandle gl_context;
	/// @brief Actual version of the created context
	GlVersion gl_version = GlVersion::Unspecified;
	// glm::vec2 dpi_scaling {};
	glm::ivec2 window_size {};
	glm::ivec2 framebuffer_size {};
};

struct SdlPreset {
	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif // include guard
