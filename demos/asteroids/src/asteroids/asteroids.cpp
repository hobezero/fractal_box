#include "asteroids/asteroids.hpp"

#include <glad/glad.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>

#include "fractal_box/core/random.hpp"
#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/graphics/core_graphics_preset.hpp"
#include "fractal_box/graphics/debug_draw_preset.hpp"
#include "fractal_box/platform/sdl_preset.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/ui/dev_ui_preset.hpp"
#include "fractal_box/ui/dimgui_preset.hpp"
#include "asteroids/game_consts.hpp"
#include "asteroids/game_systems.hpp"
#include "asteroids/game_types.hpp"
#include "asteroids/game_ui.hpp"
#include "asteroids/graphics_systems.hpp"
#include "asteroids/setup_systems.hpp"

namespace aster {

using namespace std::chrono_literals;

void AsteroidsPreset::build(fr::Runtime& runtime) {
	std::random_device rand_device;

	runtime
		.add_part(fr::SdlOptions{
			.window_title = "Asteroids",
			.window_size = {1600, 1000},
			.window_flags = fr::default_sdl_window_flags | fr::SdlWindowFlag::Resizable
		})
		.add_part(fr::LoopConfig{
			.step_delta = fr::RawDuration{1s} / 60,
			.min_app_delta = 100us,
			.max_app_delta = 100ms,
			.fixed_delta = 18ms,
			.time_scale = 1.,
			.profiler_refresh_period = 750ms
		})
		.add_preset(fr::CorePreset{})
		.add_preset(fr::SdlPreset{})
		.add_preset(fr::CoreGraphicsPreset{})
		.add_preset(fr::DebugDrawPreset{})
		.add_preset(fr::DImGuiPreset{})
		.add_preset(fr::DevUiPreset{})
		.add_preset(GameUiPreset{})

		.add_part(fr::Input{})
		.add_part(Prng{fr::RdSeedSequence(rand_device)})
		.add_part(GameClock{})
		.add_part(PhysicsClock{})
		.add_part(GameStatus{})
		.add_part(World{})
		.add_part(Players{})
		.add_part(fr::CollisionMatrix{})
		.add_part(CollisionData{})
		.add_part(MainCamera{})
		.add_part(WorldBounds{})
		.add_part(GameScore{})
		.add_part(GameConsts::make())

		.add_system<fr::SetupPhase, ActionSetupSystem>()
		.add_system<fr::SetupPhase, ResourceLoadingSystem>()
		.add_system<fr::SetupPhase, AppSetupSystem>()
		.add_system<fr::SetupPhase, GameSetupSystem>()

		.add_system<fr::FrameStartPhase, WindowMessagesRenderSystem>()
		.add_system<fr::LoopPreparePhase, ActionsToMessagesSystem>()
		.add_system<fr::LoopPreparePhase, GameStatusSystem>()
		.add_system<fr::LoopPreparePhase, WindowMessagesSystem>()
		.add_system<fr::LoopPreparePhase, ShipInputSystem>()

		.add_system<fr::FixedUpdateEarlyPhase, UpdatePhysicsClockSystem>()
		.add_system<fr::FixedUpdateMainPhase, MovementSystem>()
		.add_system<fr::FixedUpdateMainPhase, ShootingSystem>()
		.add_system<fr::FixedUpdateMainPhase, PhysicsSystem>()
		.add_system<fr::FixedUpdateMainPhase, ParentTransformSystem>()
		.add_system<fr::FixedUpdateMainPhase, BoundsSystem>()
		.add_system<fr::FixedUpdateMainPhase, CollisionSystem>()
		.add_system<fr::FixedUpdateMainPhase, CollisionResolutionSystem>()

		.add_system<fr::LoopUpdateEarlyPhase, UpdateGameClockSystem>()
		.add_system<fr::LoopUpdateMainPhase, ExpirationSystem>()
		.add_system<fr::LoopUpdateMainPhase, SpawnSystem>()

		.add_system<fr::LoopRenderEarlyPhase, ClearFramebufferSystem>()
		.add_system<fr::LoopRenderMainPhase, VfxSystem>()
		.add_system<fr::LoopRenderMainPhase, RenderSystem>()

		.add_system<fr::FrameRenderMainPhase, DebugDrawRenderSystem>()

		.add_system<fr::FrameEndPhase, WorldCleanupSystem>()
	;
}

} // namespace aster
