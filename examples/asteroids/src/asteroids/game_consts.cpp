#include "asteroids/game_consts.hpp"

#include "fractal_box/core/platform.hpp"

FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS

namespace aster {

using namespace std::chrono_literals;

auto GameConsts::make() -> GameConsts {
	return {
		.ship = {
			.larger_side = 40.f,
			.z_index = 0.5f,
			.mass = 5.f,
			.strafe_speed = 100.f,
			.rotation_speed = fr::FRad{2.f},
			.engine_thrust = 1020.f,
			.engine_reverse_thrust = 700.f,
			.gun_cooldown = 25ms,
		},
		.plume = {
			.z_index = 0.3f,
		},
		.asteroid = {
			.levels = {
				{.radius = 10.f, .hp = 30, .reward = 8},
				{.radius = 20.f, .hp = 80, .reward = 15},
				{.radius = 30.f, .hp = 130, .reward = 25},
			},
			.mass = 80.f,
			.max_rotation_speed = fr::FDeg{180},
			.speed = 70.f,
			.fragment_speed_variance = 20.f,
			.z_index = 0.2f,
		},
		.asteroid_spawner = {
			.cooldown = 6s,
		},
		.camera = {
			.smaller_viewport_side = 430.f,
		},
		.bullet = {
			.radius = 2.f,
			.size = {2.f, 2.f},
			.mass = 0.2f,
			.speed = 250.f,
			.damage = 6,
			.z_index = 0.6f,
			.time_to_live = 3s,
		},
		.world = {
			.size = {650.f, 400.f},
			.background = {0.04f, 0.08f, 0.13f, 1.f},
		},
	};
}

} // namespace aster
