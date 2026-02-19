#ifndef ASTEROIDS_GAME_TYPES_HPP
#define ASTEROIDS_GAME_TYPES_HPP

/// @file
/// @brief Description of components and the world data structure.
///
/// Our world complies with SI units. So assume 1 unit of distance is equal to 1 meter and 1 unit
/// of time is 1 second.

#include <glm/vec2.hpp>

#include "fractal_box/components/camera.hpp"
#include "fractal_box/components/collision.hpp"
#include "fractal_box/core/angle.hpp"
#include "fractal_box/core/chrono_types.hpp"
#include "fractal_box/core/random.hpp"
#include "fractal_box/core/shapes.hpp"
#include "fractal_box/core/timeline.hpp"
#include "fractal_box/platform/input.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/runtime/world.hpp"

namespace aster {

using World = fr::World<>;
using Entity = World::Entity;

/// @brief A rigid physical object
struct Body {
	/// @brief Velocity vector in m/sec
	glm::vec2 velocity {};
	/// @brief Mass in kg
	float mass = 1.f;
	/// @brief Angular spin velocity in rad/sec
	fr::FRad angular_velocity {};
	/// @brief Force vector in newtons
	glm::vec2 force {};
};

struct BodySubstepped {
	fr::FDuration step_delta;
	Body body;
};

/// @brief The entity will be destroyed at the end of the frame
struct ToBeDestroyed { };

struct AnchoredCamera: public fr::FreeCamera {
	Entity anchor;
};

struct Gun {
	int bullet_damage = 0;
	fr::FDuration cooldown {};
	fr::FDuration untill_reload {};
};

struct DamageInflictor {
	int damage = 0;
};

struct AsteroidSpawner {
	fr::FDuration cooldown {};
	fr::FDuration untill_spawn {};
};

struct Asteroid {
	int level = 0;
	int hp = 0;
};

struct Expirable {
	/// @brief Time in seconds till destruction
	fr::FDuration time_to_live {};
};

/// @todo TODO: Fixed update improvements. Calculate how strong turn/burn input is based on the
/// number of frames when uses presses a button divided by total number of frames between fixed
/// updates. On the other hand, activate fire-and-forget-style inputs (e.g. fire) as long as user
/// pressed a button at any point since the last fixed update
struct ShipControl {
	/// -1 is left, +1 is right
	float turn = 0.f;
	float engine_burn_main = 0.f;
	float engine_burn_reverse = 0.f;
	bool fire = false;
};

struct Ship {
	Entity plume;
	ShipControl control {};
};

enum class PlayerId: int { };

struct Player {
	PlayerId id;
};

inline constexpr auto action_turn_left = fr::Input::make_action("turn_left");
inline constexpr auto action_turn_right = fr::Input::make_action("turn_right");
inline constexpr auto action_engine_burn = fr::Input::make_action("engine_burn");
inline constexpr auto action_engine_reverse_burn = fr::Input::make_action("engine_reverse_burn");
inline constexpr auto action_fire = fr::Input::make_action("fire");

inline constexpr auto action_start_game = fr::Input::make_action("start_game");
inline constexpr auto action_pause_game = fr::Input::make_action("pause_game");
inline constexpr auto action_resume_game = fr::Input::make_action("resume_game");
inline constexpr auto action_toggle_pause = fr::Input::make_action("toggle_pause");
inline constexpr auto action_end_game = fr::Input::make_action("end_game");
inline constexpr auto action_restart_game = fr::Input::make_action("restart_game");

inline constexpr auto action_accept = fr::Input::make_action("accept");

struct GameScore {
	int value = 0;
};

struct Players {
	Entity player1_id {};
};

struct WorldBounds {
	fr::FAaRect value {};
};

struct WorldTime {
	fr::SteadyTimePoint value {};
};

struct MainCamera {
	AnchoredCamera value {};
};

struct Prng {
public:
	using Engine = fr::DefaultRandomEngine;
	template<class SeedSeq>
	explicit
	Prng(SeedSeq&& seed_seq):
		engine{seed_seq}
	{ }

public:
	Engine engine;
};

struct PhysicsClock: public fr::Stopwatch<PhysicsClock, struct GamePhysicsEpochTag> { };
struct GameClock: public fr::Stopwatch<GameClock, struct GameLogicEpochTag> { };

/// @brief Poor man's state machine
enum class GameStatus: uint8_t {
	NoGame,
	Running,
	Paused,
	GameOver,
};

inline constexpr
auto to_string_view(GameStatus status) noexcept -> std::string_view {
	using enum GameStatus;
	switch (status) {
		case NoGame: return "No Game";
		case Running: return "Running";
		case Paused: return "Paused";
		case GameOver: return "Game Over";
	}
	FR_UNREACHABLE();
}

struct ReqGameStart: fr::TickAtLoopUpdate { };
struct ReqGamePause: fr::TickAtLoopUpdate { };
struct ReqGameResume: fr::TickAtLoopUpdate { };
struct ReqGameTogglePause: fr::TickAtLoopUpdate { };
struct ReqEndGame: fr::TickAtLoopUpdate { };
struct ReqAccept: fr::TickAtLoopUpdate { };

using GameStatusRequests = fr::MpList<
	ReqGameStart,
	ReqGamePause,
	ReqGameResume,
	ReqGameTogglePause,
	ReqEndGame,
	ReqAccept
>;

struct CollisionData {
	std::vector<fr::Collision<Entity>> collisions;
};

} // namespace aster
#endif // include guard
