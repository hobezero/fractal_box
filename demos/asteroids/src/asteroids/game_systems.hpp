#ifndef ASTEROIDS_GAME_SYSTEMS_HPP
#define ASTEROIDS_GAME_SYSTEMS_HPP

#include "fractal_box/physics/collision.hpp"
#include "fractal_box/graphics/debug_draw_preset.hpp"
#include "fractal_box/platform/sdl_preset.hpp"
#include "fractal_box/runtime/runtime.hpp"
#include "fractal_box/runtime/system_utils.hpp"
#include "asteroids/game_consts.hpp"
#include "asteroids/game_types.hpp"
#include "asteroids/resources.hpp"

namespace aster {

struct GameRunningSystemBase {
	static constexpr auto condition = fr::InEnumState{GameStatus::Running};
};

struct ActionsToMessagesSystem {
	static
	void run(fr::Input& input, fr::MessageListWriter<GameStatusRequests>& writer);
};

struct GameStatusSystem {
	static
	void run(
		World& world,
		Players& players,
		fr::CollisionMatrix& collision_matrix,
		MainCamera& camera,
		WorldBounds& bounds,
		GameScore& score,
		GameResources& resources,
		const GameConsts& consts,
		fr::SdlData& sdl,
		const fr::AppClock& app_clock,
		fr::MessageListReader<GameStatusRequests>& messages,
		GameStatus& game_status
	);
};

struct WindowMessagesSystem {
	static
	void run(
		fr::MessageListReader<fr::WindowMessages>& messages,
		MainCamera& camera
	);
};

struct ShipInputSystem: GameRunningSystemBase {
	static
	void run(World& world, const Players& players, const fr::Input& input);
};

struct UpdatePhysicsClockSystem: GameRunningSystemBase {
	static
	void run(const fr::FixedClock& fixed_clock, PhysicsClock& physics_clock);
};

struct MovementSystem: GameRunningSystemBase {
	static
	void run(World& world, const GameConsts& consts);
};

struct ShootingSystem: GameRunningSystemBase {
	static
	void run(
		World& world,
		GameResources& resources,
		const GameConsts& consts,
		const PhysicsClock& clock
	);
};

struct PhysicsSystem: GameRunningSystemBase {
	static
	void run(
		World& world,
		const WorldBounds& bounds,
		PhysicsClock& clock,
		fr::DebugDrawAdHoc& debug_draw
	);
};

struct ParentTransformSystem: GameRunningSystemBase {
	static
	void run(World& world);
};

struct BoundsSystem: GameRunningSystemBase {
	static
	void run(World& world);
};

struct CollisionSystem: GameRunningSystemBase {
	static
	void run(
		World& world,
		CollisionData& collision_data,
		const fr::CollisionMatrix& collision_matrix
	);
};

struct CollisionResolutionSystem: GameRunningSystemBase {
	static
	void run(
		World& world,
		GameScore& score,
		GameResources& resources,
		const CollisionData& collision_data,
		const GameConsts& consts,
		Prng& prng,
		fr::MessageWriter<ReqEndGame>& msg_writer
	);
};

struct UpdateGameClockSystem: GameRunningSystemBase {
	static
	void run(const fr::AppClock& app_clock, GameClock& game_clock);
};

struct ExpirationSystem: GameRunningSystemBase {
	static
	void run(World& world, const GameClock& clock);
};

/// @todo TODO: Move to FixedUpdatePhase?
struct SpawnSystem: GameRunningSystemBase {
	static
	void run(
		World& world,
		const WorldBounds& bounds,
		GameResources& resources,
		const GameConsts& consts,
		const GameClock& clock,
		Prng& prng
	);
};

struct WorldCleanupSystem {
	static
	void run(World& world);
};

} // namespace aster
#endif // include guard
