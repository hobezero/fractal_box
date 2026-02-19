#ifndef FRACTAL_BOX_UI_DIMGUI_PRESET_HPP
#define FRACTAL_BOX_UI_DIMGUI_PRESET_HPP

#include <imgui.h>

#include "fractal_box/runtime/runtime.hpp"

namespace fr {

struct DImGuiData {
	// Second context is the easisest way to separate game UI and developer/editor UI. Game UI
	// is updated and rendered to the offscreen framebuffer at "loop" phases which can be
	// temporarily disabled. Developer UI, on the other hand, should be updated and rendered to the
	// default framebuffer every single frame.
	ImGuiContext* app_context = nullptr;
	ImGuiContext* dev_context = nullptr;
};

struct DImGuiPreset {
	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif
