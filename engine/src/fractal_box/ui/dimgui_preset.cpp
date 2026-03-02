#include "fractal_box/ui/dimgui_preset.hpp"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_backends/imgui_impl_opengl3.h>
#include <imgui_backends/imgui_impl_sdl2.h>

#include "fractal_box/core/logging.hpp"
#include "fractal_box/platform/sdl_preset.hpp"
#include "fractal_box/runtime/core_preset.hpp"

namespace fr {

struct DImGuiInitSystem {
	static
	auto run(Runtime& runtime, const SdlData& sdl) -> ErrorOr<> {
		// On failure, `IMGUI_CHECKVERSION()` either crashes or returns `false` depending on
		// whether `IM_ASSERT`s are enabled
		if (!IMGUI_CHECKVERSION())
			return make_error_fmt(Errc::ImGuiError, "Incompatible Dear ImGui version");

		runtime.add_part(DImGuiData{
			.app_context = make_context(sdl),
			.dev_context = make_context(sdl),
		});
		FR_LOG_INFO_MSG("Dear ImGui initialized");

		return {};
	}

private:
	static
	auto make_context(const SdlData& sdl) -> ImGuiContext* {
		auto* const ctx = ImGui::CreateContext();
		ImGui::SetCurrentContext(ctx);

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui_ImplSDL2_InitForOpenGL(sdl.window.get(), sdl.gl_context.get());
		// NOTE: `gl_shader_version_line` actually returns a null-terminated string
		ImGui_ImplOpenGL3_Init(gl_shader_version_line(sdl.gl_version).data());

		return ctx;
	}
};

/// @todo
///   TODO: Make sure that we run all global shutdown systems in a correct order.
///   This one should be executed BEFORE SDL cleanup
struct DImGuiShutdownSystem {
	static
	void run(const DImGuiData& imgui) {
		kill_context(imgui.app_context);
		kill_context(imgui.dev_context);
		FR_LOG_INFO_MSG("Dear ImGui uninitialized");
	}

private:
	static
	void kill_context(ImGuiContext* context) {
		ImGui::SetCurrentContext(context);
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext(context);
	}
};

using DImGuiContextMember = ImGuiContext* DImGuiData::*;

template<DImGuiContextMember Context>
struct DImGuiFrameStartSystemBase {
	static
	void run(const DImGuiData& imgui, MessageReader<SDL_Event>& messages) {
		ImGui::SetCurrentContext(imgui.*Context);
		auto& io = ImGui::GetIO();

		messages.for_each([&] (const SDL_Event& event){
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type) {
				case SDL_MOUSEMOTION:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEWHEEL:
					if (io.WantCaptureMouse)
						return true;
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					if (io.WantCaptureKeyboard)
						return true;
					break;
				default: break;
			}
			return false;
		});

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
};

struct DImGuiFrameStartDevSystem: DImGuiFrameStartSystemBase<&DImGuiData::dev_context> { };
struct DImGuiFrameStartAppSystem: DImGuiFrameStartSystemBase<&DImGuiData::app_context> { };

template<DImGuiContextMember Context>
struct DImGuiRenderSystemBase {
	static
	void run(const DImGuiData& imgui) {
		ImGui::SetCurrentContext(imgui.*Context);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
};

struct DImGuiRenderDevSystem: DImGuiRenderSystemBase<&DImGuiData::dev_context> { };
struct DImGuiRenderAppSystem: DImGuiRenderSystemBase<&DImGuiData::app_context> { };

void DImGuiPreset::build(Runtime& runtime) {
	runtime
		.add_system<SetupPhase, DImGuiInitSystem>()
		.add_system<ShutdownPhase, DImGuiShutdownSystem>()
		.add_system<FrameStartPhase, DImGuiFrameStartDevSystem>()
		.add_system<LoopPreparePhase, DImGuiFrameStartAppSystem>()
		.add_system<LoopRenderLatePhase, DImGuiRenderAppSystem>()
		.add_system<FrameRenderLatePhase, DImGuiRenderDevSystem>()
	;
}

} // namespace fr
