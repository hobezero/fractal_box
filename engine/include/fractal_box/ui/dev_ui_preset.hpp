#ifndef FRACTAL_BOX_UI_DEV_UI_PRESET_HPP
#define FRACTAL_BOX_UI_DEV_UI_PRESET_HPP

#include <string>
#include <vector>

#include <imgui.h>

#include "fractal_box/platform/input.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/runtime/runtime.hpp"

namespace fr {

struct DevToolsUiPhase { };

inline constexpr auto action_loop_toggle = Input::make_action("loop_toggle");
inline constexpr auto action_loop_step = Input::make_action("loop_step");
inline constexpr auto action_dev_ui_toggle = Input::make_action("dev_ui_toggle");
inline constexpr auto action_debug_draw_toggle = Input::make_action("debug_draw_toggle");

struct ReqDevUiToggle: TickAtFrameEnd { };
struct ReqDebugDrawToggle: TickAtFrameEnd { };

using DevUiRequests = MpList<
	ReqDevUiToggle,
	ReqDebugDrawToggle
>;

struct DevUiConsts {
	float main_font_size = 16.f;
	float metrics_font_size = 14.f;

	struct Shortcuts {
		ScanCode loop_toggle = ScanCode::F9;
		ScanCode loop_step = ScanCode::F10;
		ScanCode dev_ui_toggle = ScanCode::F5;
		ScanCode debug_draw_toggle = ScanCode::F6;
	} shortcuts;
};

class DevTool {
public:
	template<c_system System>
	explicit constexpr
	DevTool(InPlaceAsInit<System>):
		_display_name(refl_display_name<System>),
		_system_idx{SystemTypeIdx::of<System>}
	{ }

	constexpr
	auto display_name() const noexcept -> const std::string& { return _display_name; }

	template<c_mutable Self>
	constexpr
	auto set_display_name(this Self&& self, std::string display_name) noexcept -> Self&& {
		self._display_name = std::move(display_name);
		return std::forward<Self>(self);
	}

	constexpr
	auto system_idx() const noexcept -> SystemTypeIdx { return _system_idx; }

private:
	std::string _display_name;
	SystemTypeIdx _system_idx;
};

class DevUi {
public:
	template<c_system System>
	void add_tool(Runtime& runtime, bool shown_on_start = false) {
		runtime.add_system<DevToolsUiPhase>(AnySystem{in_place_as<System>}
			.set_enabled(shown_on_start));
		_tools.emplace_back(in_place_as<System>);
	}

	auto tools() const noexcept -> const std::vector<DevTool>& { return _tools; }

public:
	bool window_shown = false;
	ImFont* main_font = nullptr;
	ImFont* metrics_font = nullptr;

private:
	std::vector<DevTool> _tools;
};

struct ProfilerUiConfig {
	static
	auto make() noexcept -> ProfilerUiConfig;

public:
	int stat_prefix_chars;
	int stat_precision;
	int spf_prefix_chars;
	int spf_precision;
	ImVec4 hot_color_root;
	ImVec4 hot_color_branch;
	ImVec4 hot_color_leaf;
	float hot_color_exponent;
};

struct DevUiPreset {
	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif
