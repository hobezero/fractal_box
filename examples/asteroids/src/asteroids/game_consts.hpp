#ifndef ASTEROIDS_GAME_CONSTS_HPP
#define ASTEROIDS_GAME_CONSTS_HPP

/// @file
/// @brief Gameplay and context parameters/constants go here

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "fractal_box/core/angle.hpp"
#include "fractal_box/core/chrono_types.hpp"

namespace aster {

struct GameConsts {
	static
	auto make() -> GameConsts;

	struct Ship {
		float larger_side;
		float z_index;
		float mass;
		float strafe_speed;
		fr::FRad rotation_speed;
		float engine_thrust;
		float engine_reverse_thrust;
		fr::FDuration gun_cooldown;
	} ship;

	struct Plume {
		float z_index;
	} plume;

	struct Asteroid {
		auto max_level() const noexcept -> int { return static_cast<int>(this->levels.size()) - 1; }

	public:
		struct LevelInfo {
			float radius;
			int hp;
			int reward;
		};

		std::vector<LevelInfo> levels;
		float mass;
		fr::FRad max_rotation_speed;
		float speed;
		float fragment_speed_variance;
		float z_index;
	} asteroid;

	struct AsteroidSpawner {
		fr::FDuration cooldown;
	} asteroid_spawner;

	struct Camera {
		float smaller_viewport_side;
	} camera;

	struct Bullet {
		float radius;
		glm::vec2 size;
		float mass;
		float speed;
		int damage;
		float z_index;
		fr::FDuration time_to_live;
	} bullet;

	struct World {
		glm::vec2 size;
		glm::vec4 background;
	} world;
};

} // namespace aster
#endif // include guard
