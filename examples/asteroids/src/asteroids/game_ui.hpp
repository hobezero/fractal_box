#ifndef ASTEROIDS_GAME_UI_HPP
#define ASTEROIDS_GAME_UI_HPP

#include <glm/vec2.hpp>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/runtime/runtime.hpp"

namespace aster {

struct GameUiConsts {
	static
	auto make() noexcept -> GameUiConsts;

public:
	float hud_large_font_size;
	float hud_medium_font_size;
	float hud_small_font_size;

	float widget_height;
	glm::vec2 button_size;
	float dialog_padding;
	glm::vec2 game_paused_dialog_size;
	glm::vec2 game_over_dialog_size;

	struct Colors {
		void apply() const;

	public:
		uint32_t button;
		uint32_t button_hovered;
		uint32_t button_active;
		uint32_t bg;
		uint32_t defeat;
	} colors;

};

struct GameUiPreset {
	static
	void build(fr::Runtime& runtime);
};

} // namespace aster
#endif
