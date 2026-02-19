#include "fractal_box/platform/sdl_preset.hpp"

#include <glad/glad.h> // DO NOT REORDER
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>

#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/platform/input.hpp"
#include "fractal_box/runtime/core_preset.hpp"

namespace fr {

static constinit auto sdl_refcount = int{0};

static
auto query_window_size(SDL_Window* window) noexcept -> glm::ivec2 {
	FR_ASSERT(window);
	auto win_width = int{};
	auto win_height = int{};
	SDL_GetWindowSize(window, &win_width, &win_height);
	return {win_width, win_height};
}

static
auto query_framebuffer_size(SDL_Window* window) noexcept -> glm::ivec2 {
	FR_ASSERT(window);
	auto fb_width = int{};
	auto fb_height = int{};
	SDL_GL_GetDrawableSize(window, &fb_width, &fb_height);
	return {fb_width, fb_height};
}

auto SdlGuard::make(uint32_t init_flags) -> ErrorOr<SdlGuard> {
	if (SDL_Init(init_flags) != 0)
		return make_error_fmt(Errc::SdlError, "Failed to initialize SDL: {}", SDL_GetError());
	FR_LOG_INFO_MSG("SDL initialized");
	return SdlGuard{adopt, init_flags};
}

SdlGuard::~SdlGuard() {
	if (_subsystem_flags.is_default())
		return;

	SDL_QuitSubSystem(*_subsystem_flags);
	FR_LOG_INFO("SDL subsystems (flags: {:#x}) uninitialized", *_subsystem_flags);

	// NOTE: SDL2 refcounts initialization state internally, so it should be (probably) safe to
	// create multiple `Runtime` objects which call `SDL_Quit` multiple times. However, by doing so
	// we might deinitialize SDL subsystems that others are still using
	if (sdl_refcount == 0) {
		SDL_Quit();
		FR_LOG_INFO_MSG("SDL uninitialized");
	}
}

static
void GLAPIENTRY gl_message_callback(
	[[maybe_unused]] GLenum source,
	GLenum type,
	[[maybe_unused]] GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	[[maybe_unused]] const void* user_param
) {
	using namespace std::string_view_literals;
	if (type == GL_DEBUG_TYPE_ERROR) {
		FR_LOG_ERROR("OpenGL error: '{}'. Type: {:#x}, severity: {:#x}",
			std::string_view(message, static_cast<size_t>(length)), type, severity);
	}
	else {
		FR_LOG_TRACE("OpenGL message: '{}'. Type: {:#x}, severity: {:#x}",
			std::string_view(message, static_cast<size_t>(length)), type, severity);
	}
}

struct SdlInitSystem {
	static
	auto run(const SdlOptions& options, Runtime& runtime) -> ErrorOr<> {
		// Based on https://github.com/Sibras/OpenGL4-Tutorials/blob/main/Tutorial1/Main.cpp

		auto sdl = SdlGuard::make(SDL_INIT_VIDEO);
		if (!sdl)
			return extract_unexpected(std::move(sdl));
		FR_LOG_INFO("Initialized SDL with subsystem flags: {:#x}", sdl->subsystem_flags());

		const auto gl_version_requested = GlVersionPair{options.gl_version};
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_version_requested.major());
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_version_requested.minor());
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	#if FR_IS_DEBUG_BUILD
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	#endif

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, options.with_doublebuffer ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, options.depth_size_bits);

		auto window = SdlWindowHandle{SDL_CreateWindow(
			options.window_title.c_str(),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			options.window_size.x, options.window_size.y,
			options.window_flags.raw_value()
		)};
		if (!window)
			return make_error_fmt(Errc::SdlError, "Failed to create OpenGL window: {}",
				SDL_GetError());
		FR_LOG_INFO("Created SDL window. Title: '{}', size: {}x{}, flags: {:#x}",
			options.window_title, options.window_size.x, options.window_size.y,
			options.window_flags.raw_value());

		auto gl_context = SdlGlContextHandle{SDL_GL_CreateContext(window.get())};
		if (!gl_context)
			return make_error_fmt(Errc::SdlError, "Failed to create OpenGL context: {}",
				SDL_GetError());
		SDL_GL_MakeCurrent(window.get(), gl_context.get());

		auto ok = gladLoadGLLoader(&SDL_GL_GetProcAddress);
		if (ok == 0)
			return make_error_fmt(Errc::GladError, "Failed to load OpenGL functions");
		FR_LOG_INFO("Loaded OpenGL function pointers. Context version: OpenGL {}.{}",
			GLVersion.major, GLVersion.minor);

		if (SDL_GL_SetSwapInterval(to_underlying(options.swap_interval)) == -1) {
			FR_LOG_ERROR("Failed to set swap interval to {}",
				to_string_view(options.swap_interval));
			if (options.swap_interval == SwapInterval::AdaptiveVSync) {
				// Retry regular vsync as a fallback
				SDL_GL_SetSwapInterval(to_underlying(SwapInterval::VSync));
			}
		}

		if (options.with_gl_debug_output) {
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(gl_message_callback, nullptr);
		}

		const auto win_size = query_window_size(window.get());
		const auto fb_size = query_framebuffer_size(window.get());
		runtime.add_part(SdlData{
			.guard = *std::move(sdl),
			.window = std::move(window),
			.gl_context = std::move(gl_context),
			.gl_version = GlVersionPair{GraphicsApiKind::OpenGl, GLVersion.major, GLVersion.minor}
				.combine(),
			.window_size = win_size,
			.framebuffer_size = fb_size,
		});
		return {};
	}
};

struct SdlPollEventsSystem {
	static
	void run(
		Input& input,
		SdlData& sdl,
		MessageWriter<SDL_Event>& sdl_messages,
		MessageListWriter<ReqLoopQuit, WindowMessages>& generic_messages
	) {
		SDL_Event event;
		while (SDL_PollEvent(&event) == SDL_TRUE) {
			switch (event.type) {
				case SDL_QUIT: {
					generic_messages.push(ReqLoopQuit{});
					break;
				}
				case SDL_WINDOWEVENT: {
					handle_win_event(event.window, sdl, generic_messages);
					break;
				}
				default: break;
			}

			sdl_messages.push(event);
			input.accept_event(event);
		}
		input.emit_down_actions();
	}

private:
	static
	void handle_win_event(
		const SDL_WindowEvent& event,
		SdlData& sdl,
		MessageListWriter<ReqLoopQuit, WindowMessages>& messages
	) {
		switch (event.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED: {
				sdl.window_size = query_window_size(sdl.window.get());
				sdl.framebuffer_size = query_framebuffer_size(sdl.window.get());
				FR_LOG_INFO("SDL: Window resized to ({}, {})", sdl.window_size.x,
					sdl.window_size.y);
				messages.push(WindowResized{
					.window_size = sdl.window_size,
					.framebuffer_size = sdl.framebuffer_size
				});
				break;
			}
			// TODO: Decide what to do with `SDL_WINDOWEVENT_CLOSE` (hint: consider multiple
			// windows)
			default: break;
		}
	}
};

struct ClearInputSystem {
	static
	void run(Input& input) {
		input.clear_actions();
	}
};

struct SdlSwapWindowBuffersSystem {
	static
	void run(const SdlData& sdl) {
		SDL_GL_SwapWindow(sdl.window.get());
	}
};

void SdlPreset::build(Runtime& runtime) {
	runtime
		.add_system<SetupPhase, SdlInitSystem>()
		.add_system<FrameStartPhase, SdlPollEventsSystem>()
		.add_system<FrameLastPhase, ClearInputSystem>()
		.add_system<FrameLastPhase, SdlSwapWindowBuffersSystem>()
	;
}

} // namespace fr
