#include "asteroids/game_ui.hpp"

#include <cmrc/cmrc.hpp>
#include <imgui.h>

#include "fractal_box/resources/resource_utils.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/ui/dimgui_preset.hpp"
#include "fractal_box/ui/dimgui_utils.hpp"
#include "asteroids/game_types.hpp"

CMRC_DECLARE(aster);

FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS

namespace aster {

using namespace std::string_view_literals;

auto GameUiConsts::make() noexcept -> GameUiConsts {
	return {
		.hud_large_font_size = 32.f,
		.hud_medium_font_size = 18.f,
		.hud_small_font_size = 14.f,

		.widget_height = 36.f,
		.button_size = {96.f, 36.f},
		.dialog_padding = 16.f,
		.game_paused_dialog_size = {320.f, 240.f},
		.game_over_dialog_size = {320.f, 240.f},

		.colors = {
			.button = 0xFF6100FFu,
			.button_hovered = 0xFF721CFFu,
			.button_active = 0x6700BFFFu,
			.bg = 0x141414F5u,
			.defeat = 0xFC2020FF,
		}
	};
}

void GameUiConsts::Colors::apply() const {
	auto& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Button] = fr::rgba_to_im_vec4(this->button);
	style.Colors[ImGuiCol_ButtonHovered] = fr::rgba_to_im_vec4(this->button_hovered);
	style.Colors[ImGuiCol_ButtonActive] = fr::rgba_to_im_vec4(this->button_active);
	style.Colors[ImGuiCol_WindowBg] = fr::rgba_to_im_vec4(this->bg);
}

struct GameUiData {
	ImFont* hud_large_font;
	ImFont* hud_medium_font;
	ImFont* hud_small_font;
};

struct GameUiInitSystem {
	static
	auto run(fr::Runtime& runtime, const GameUiConsts& consts) -> fr::ErrorOr<> {
		const auto fs = cmrc::aster::get_filesystem();
		const auto scale = 1.f; // TODO: DPI scaling
		const auto oxanium = fr::get_resource_data(fs, "fonts/Oxanium-SemiBold.ttf");
		auto* hud_large_font = fr::make_im_font_from_data(oxanium, scale * consts.hud_large_font_size);
		auto* hud_medium_font = fr::make_im_font_from_data(oxanium,
			scale * consts.hud_medium_font_size);
		auto* hud_small_font = fr::make_im_font_from_data(oxanium, scale * consts.hud_small_font_size);
		if (!hud_large_font)
			return make_error(fr::Errc::ImGuiError, "GameUi: failed to create large HUD font");
		if (!hud_medium_font)
			return make_error(fr::Errc::ImGuiError, "GameUi: failed to create medium HUD font");
		if (!hud_small_font)
			return make_error(fr::Errc::ImGuiError, "GameUi: failed to create small HUD font");

		runtime.add_part(GameUiData{
			.hud_large_font = hud_large_font,
			.hud_medium_font = hud_medium_font,
			.hud_small_font = hud_small_font,
		});
		return {};
	}
};

struct GameUiUpdateSystem {
	static
	void run(
		const fr::DImGuiData& imgui,
		const GameUiConsts& consts,
		const GameUiData& ui,
		GameStatus status,
		const GameScore& score,
		fr::MessageListWriter<GameStatusRequests>& messages
	) {
		ImGui::SetCurrentContext(imgui.app_context);
		ImGui::StyleColorsDark();
		consts.colors.apply();
		switch (status) {
			case GameStatus::NoGame:
				break;
			case GameStatus::Running:
				score_panel(ui, score);
				break;
			case GameStatus::Paused:
				score_panel(ui, score);
				game_paused_dialog(ui, consts, messages);
				break;
			case GameStatus::GameOver:
				game_over_dialog(ui, consts, score, messages);
				break;
		}
	}

private:
	static
	void score_panel(
		const GameUiData& ui,
		const GameScore& score
	) {
		constexpr auto margin = 4.f;
		ImGui::SetNextWindowPos({ImGui::GetMainViewport()->GetCenter().x, margin},
			ImGuiCond_Always, {0.5f, 0.f});
		const auto window = fr::im_scoped_window("ScorePanel", nullptr, fr::im_overlay_win_flags
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_NoMove);
		if (!window)
			return;
		const auto font = fr::im_scoped_font(ui.hud_medium_font);
		fr::im_text(fmt::format("Score: {}", score.value));
	}

	static
	void game_paused_dialog(
		const GameUiData& ui,
		const GameUiConsts& consts,
		fr::MessageListWriter<GameStatusRequests>& messages
	) {
		ImGui::SetNextWindowSize(fr::to_im_vec(consts.game_paused_dialog_size));
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always,
			{0.5f, 0.5f});
		const auto window = fr::im_scoped_window("GamePausedDialog", nullptr,
			fr::im_overlay_win_flags);
		// TODO: BeginPopup/BeginModal
		{
			const auto font = fr::im_scoped_font(ui.hud_large_font);
			const auto text = "Game Paused"sv;
			const auto text_size = fr::im_calc_text_size(text);
			ImGui::SetCursorPosX(0.5f * (consts.game_paused_dialog_size.x - text_size.x));
			ImGui::SetCursorPosY(consts.dialog_padding);
			fr::im_text(text);
		}
		{
			const auto font = fr::im_scoped_font(ui.hud_medium_font);
			const auto text = "Press P to resume"sv;
			const auto text_size = fr::to_vec(fr::im_calc_text_size(text));
			ImGui::SetCursorPos(fr::to_im_vec(0.5f * (consts.game_paused_dialog_size - text_size)));
			fr::im_text(text);
		}
		{
			const auto font = fr::im_scoped_font(ui.hud_medium_font);
			ImGui::SetCursorPosX(0.5f * (consts.game_paused_dialog_size.x - consts.button_size.x));
			ImGui::SetCursorPosY(consts.game_paused_dialog_size.y - consts.button_size.y
				- consts.dialog_padding);
			if (ImGui::Button("Resume", fr::to_im_vec(consts.button_size))) {
				messages.push(ReqGameResume{});
			}
		}
	}

	static
	void game_over_dialog(
		const GameUiData& ui,
		const GameUiConsts& consts,
		const GameScore& score,
		fr::MessageListWriter<GameStatusRequests>& messages
	) {
		ImGui::SetNextWindowSize(fr::to_im_vec(consts.game_over_dialog_size));
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always,
			{0.5f, 0.5f});
		const auto window = fr::im_scoped_window("GameOverDialog", nullptr, fr::im_overlay_win_flags);
		{
			const auto color = fr::im_scoped_color(ImGuiCol_Text,
				fr::rgba_to_im_vec4(consts.colors.defeat));
			const auto font = fr::im_scoped_font(ui.hud_large_font);
			const auto text = "Game Over"sv;
			const auto text_size = fr::im_calc_text_size(text);
			ImGui::SetCursorPosX(0.5f * (consts.game_over_dialog_size.x - text_size.x));
			ImGui::SetCursorPosY(consts.dialog_padding);
			fr::im_text(text);
		}
		{
			const auto font = fr::im_scoped_font(ui.hud_medium_font);
			const auto text = fmt::format("Your score: {}", score.value);
			const auto text_size = fr::to_vec(fr::im_calc_text_size(text));
			ImGui::SetCursorPos(fr::to_im_vec(0.5f * (consts.game_over_dialog_size - text_size)));
			fr::im_text(text);
		}
		{
			const auto font = fr::im_scoped_font(ui.hud_medium_font);
			ImGui::SetCursorPosX(0.5f * (consts.game_over_dialog_size.x - consts.button_size.x));
			ImGui::SetCursorPosY(consts.game_over_dialog_size.y - consts.button_size.y
				- consts.dialog_padding);
			if (ImGui::Button("Try Again", fr::to_im_vec(consts.button_size))) {
				messages.push(ReqGameStart{});
			}
		}
	}
};

void GameUiPreset::build(fr::Runtime& runtime) {
	runtime
		.try_add_part(GameUiConsts::make())
		.add_system<fr::SetupPhase, GameUiInitSystem>()
		.add_system<fr::LoopRenderEarlyPhase, GameUiUpdateSystem>()
	;
}

} // namespace aster
