#ifndef ASTEROIDS_GRAPHICS_SYSTEMS_HPP
#define ASTEROIDS_GRAPHICS_SYSTEMS_HPP

#include "fractal_box/graphics/debug_draw_config.hpp"
#include "fractal_box/graphics/debug_draw_preset.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "asteroids/game_consts.hpp"
#include "asteroids/game_types.hpp"
#include "asteroids/resources.hpp"

namespace aster {

struct WindowMessagesRenderSystem {
	static
	void run(fr::MessageListReader<fr::WindowMessages>& messages);
};

struct ClearFramebufferSystem {
	static
	void run(const GameConsts& consts);
};

struct RenderSystem {
	static
	auto run(
		World& world,
		const MainCamera& camera,
		GameResources& resources
	) -> fr::ErrorOr<>;
};

struct DebugDrawRenderSystem {
	static
	auto run(
		const fr::DebugDrawConfig& config,
		const fr::DebugDrawAdHocData& adhoc,
		World& world,
		const MainCamera& camera,
		GameResources& resources
	) -> fr::ErrorOr<>;
};

struct VfxSystem {
	static
	void run(World& world);
};

} // namespace aster
#endif
