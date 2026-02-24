#include "asteroids/game_systems.hpp"

#include <glm/geometric.hpp>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/graphics/sprite.hpp"
#include "fractal_box/math/math.hpp"
#include "fractal_box/physics/collision.hpp"
#include "fractal_box/physics/numerical.hpp"
#include "fractal_box/platform/input.hpp"

#include "asteroids/game_consts.hpp"
#include "asteroids/game_types.hpp"

using namespace std::chrono_literals;

namespace aster {

static constexpr auto collider_asteroid = fr::CollisionLayer{1u};
static constexpr auto collider_player = fr::CollisionLayer{2u};
static constexpr auto collider_bullet = fr::CollisionLayer{4u};

using Nonce = uint16_t;

#if 0
/// @brief Can be used for logging to distinguish similar messages
[[nodiscard, maybe_unused]] static
auto make_nonce(DefaultRandomEngine& rand) noexcept -> Nonce {
	std::uniform_int_distribution<Nonce> dist {
		std::numeric_limits<Nonce>::min(),
		std::numeric_limits<Nonce>::max()
	};
	return dist(rand);
}
#endif

static
auto make_ship(
	World& world, GameResources& rsc, const GameConsts& consts, glm::vec2 pos = {}
) -> Entity {
	const auto ship_size = fr::proportionally_resized_max(glm::vec2{rsc.tex_spaceship.dimensions()},
		consts.ship.larger_side);
	const auto smaller_side = std::min(ship_size.x, ship_size.y);
	const auto ship = world.spawn_with(
		fr::in_place_args<fr::Transform>( pos, fr::FDeg{90}, ship_size),
		fr::in_place_args<fr::Sprite>(rsc.tex_spaceship, rsc.mesh_square_solid,
			consts.ship.z_index),
		fr::in_place_args<Body>(glm::vec2{}, consts.ship.mass),
		fr::in_place_args<fr::Collider>(collider_player, fr::FCircle{pos, 0.5f * smaller_side}),
		fr::in_place_args<Gun>(consts.bullet.damage, consts.ship.gun_cooldown,
			consts.ship.gun_cooldown)
	);

	const auto plume_size = 0.7f * glm::vec2{fr::aspect_ratio(glm::vec2{
		rsc.tex_plume.dimensions()}) * ship_size.y, ship_size.y};
	const auto plume_x = -0.5f * ship_size.x - 0.5f * plume_size.x;
	const auto plume = world.spawn_with(
		fr::in_place_args<fr::Transform>(pos, fr::FDeg{0}, plume_size),
		fr::in_place_args<fr::Sprite>(rsc.tex_plume, rsc.mesh_square_solid, consts.plume.z_index,
			false),
		fr::in_place_args<fr::ParentRelationship<Entity>>(ship,
			fr::TransRotationTransform{{plume_x, 0.f}})
	);

	world.emplace_component<Ship>(ship, plume);

	return ship;
}

static
auto make_player(
	World& world, GameResources& rsc, const GameConsts& consts, PlayerId id, glm::vec2 pos = {}
) -> Entity {
	const auto player = make_ship(world, rsc, consts, pos);
	world.emplace_component<Player>(player, id);
	return player;
}

static
auto make_asteroid(
	World& world,
	GameResources& rsc,
	const GameConsts& consts,
	Prng& prng,
	int level,
	glm::vec2 pos = {}, glm::vec2 velocity = {}
) -> Entity {
	FR_ASSERT(level <= consts.asteroid.max_level());
	std::uniform_int_distribution<size_t> tex_dist {0, rsc.tex_asteroids.size() - 1};
	const auto tex_index = tex_dist(prng.engine);
	const auto radius = consts.asteroid.levels[size_t(level)].radius;
	const auto rotation = fr::FRad{roll_uniform(prng.engine,
		-consts.asteroid.max_rotation_speed,
		consts.asteroid.max_rotation_speed)};

	return world.spawn_with(
		fr::in_place_args<fr::Transform>(pos, fr::FRad{0}, glm::vec2{2.f * radius, 2.f * radius}),
		fr::in_place_args<fr::Sprite>(rsc.tex_asteroids[tex_index], rsc.mesh_square_solid,
			consts.asteroid.z_index),
		fr::in_place_args<Body>(velocity, consts.asteroid.mass, rotation),
		fr::in_place_args<fr::Collider>(collider_asteroid, fr::FCircle{pos, radius}),
		fr::in_place_args<Asteroid>(level, consts.asteroid.levels[size_t(level)].hp)
	);
}

struct BulletSpec {
	glm::vec2 pos;
	fr::FRad rotation;
	glm::vec2 velocity;
	fr::FDuration step_delta;
};

static
auto make_bullet(
	World& world, GameResources& rsc, const GameConsts& consts, const BulletSpec& spec
) -> Entity {
	return world.spawn_with(
		fr::in_place_args<fr::Transform>(spec.pos, spec.rotation, consts.bullet.size),
		fr::in_place_args<fr::Sprite>(rsc.tex_bullet, rsc.mesh_square_solid, consts.bullet.z_index),
		fr::in_place_args<BodySubstepped>(spec.step_delta, Body{spec.velocity, consts.bullet.mass}),
		fr::in_place_args<DamageInflictor>(consts.bullet.damage),
		fr::in_place_args<fr::Collider>(collider_bullet, fr::FPoint2d{spec.pos}),
		fr::in_place_args<Expirable>(consts.bullet.time_to_live)
	);
}

static
auto make_asteroid_spawner(
	World& world, const GameConsts& consts, fr::FDuration initial_time
) -> Entity {
	return world.spawn_with(
		fr::in_place_args<AsteroidSpawner>(consts.asteroid_spawner.cooldown, initial_time)
	);
}

static
void reset_globals(
	World& world,
	Players& players,
	fr::CollisionMatrix& collision_matrix,
	MainCamera& camera,
	WorldBounds& bounds,
	GameScore& score
) {
	world.clear();
	players.player1_id = {};
	collision_matrix.clear();
	camera.value = {};
	bounds.value = {};
	score.value = 0;
}

static
void populate_world(
	World& world,
	Players& players,
	fr::CollisionMatrix& collision_matrix,
	MainCamera& camera,
	WorldBounds& bounds,
	GameResources& rsc,
	const GameConsts& consts
) {
	collision_matrix.add(collider_asteroid, collider_bullet);
	collision_matrix.add(collider_asteroid, collider_player);

	bounds.value = {-0.5f * consts.world.size, 0.5f * consts.world.size};
	camera.value.projection.view_side = consts.camera.smaller_viewport_side;

	players.player1_id = make_player(world, rsc, consts, PlayerId{1});
	make_asteroid_spawner(world, consts, 2s);
	make_asteroid_spawner(world, consts, 3s);
}

void ActionsToMessagesSystem::run(
	fr::Input& input,
	fr::MessageListWriter<GameStatusRequests>& writer
) {
	if (input.get_action(action_start_game))
		writer.push(ReqGameStart{});
	if (input.get_action(action_pause_game))
		writer.push(ReqGamePause{});
	if (input.get_action(action_resume_game))
		writer.push(ReqGameResume{});
	if (input.get_action(action_toggle_pause))
		writer.push(ReqGameTogglePause{});
	if (input.get_action(action_accept))
		writer.push(ReqAccept{});
}

static
void start_game(
	World& world,
	Players& players,
	fr::CollisionMatrix& collision_matrix,
	MainCamera& camera,
	WorldBounds& bounds,
	GameScore& score,
	GameResources& resources,
	const GameConsts& consts,
	const fr::AppClock& app_clock,
	const fr::SdlData& sdl,
	GameStatus& game_status
) {
	reset_globals(world, players, collision_matrix, camera, bounds, score);
	populate_world(world, players, collision_matrix, camera, bounds, resources, consts);
	camera.value.set_viewport(sdl.framebuffer_size);
	game_status = GameStatus::Running;
	FR_LOG_INFO("Game Started on frame #{}", app_clock.tick_id());
}

static
void pause_game(const fr::AppClock& app_clock, GameStatus& game_status) {
	if (game_status == GameStatus::Running) {
		game_status = GameStatus::Paused;
		FR_LOG_INFO("Game Paused on frame #{}", app_clock.tick_id());
	}
}

static
void resume_game(const fr::AppClock& app_clock, GameStatus& game_status) {
	if (game_status == GameStatus::Paused) {
		game_status = GameStatus::Running;
		FR_LOG_INFO("Game Resumed on frame #{}", app_clock.tick_id());
	}
}

static
void end_game(const fr::AppClock& app_clock, GameStatus& game_status) {
	if (game_status == GameStatus::Running || game_status == GameStatus::Paused) {
		game_status = GameStatus::GameOver;
		FR_LOG_INFO("Game Ended on frame #{}", app_clock.tick_id());
	}
}

void GameStatusSystem::run(
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
) {
	messages.for_each(fr::Overload{
		[&](ReqGameStart) {
			start_game(world, players, collision_matrix, camera, bounds, score, resources, consts,
				 app_clock, sdl, game_status);
			return true;
		},
		[&](ReqGamePause) {
			pause_game(app_clock, game_status);
			return true;
		},
		[&](ReqGameResume) {
			resume_game(app_clock, game_status);
			return true;
		},
		[&](ReqGameTogglePause) {
			if (game_status == GameStatus::Running)
				pause_game(app_clock, game_status);
			else if (game_status == GameStatus::Paused)
				resume_game(app_clock, game_status);
			return true;
		},
		[&](ReqEndGame) {
			end_game(app_clock, game_status);
			return true;
		},
		[&](ReqAccept) {
			if (game_status == GameStatus::Paused) {
				resume_game(app_clock, game_status);
				return true;
			}
			if (game_status == GameStatus::GameOver) {
				start_game(world, players, collision_matrix, camera, bounds, score, resources,
					consts, app_clock, sdl, game_status);
				return true;
			}
			return false;
		}
	});
}

void WindowMessagesSystem::run(
	fr::MessageListReader<fr::WindowMessages>& messages,
	MainCamera& camera
) {
	messages.for_each(fr::Overload{
		[&](const fr::WindowResized& msg) {
			camera.value.set_viewport(msg.framebuffer_size);
		},
		[](auto&&) { }
	});
}

void ShipInputSystem::run(World& world, const Players& players, const fr::Input& input) {
	if (!world.contains(players.player1_id))
		return;
	auto* ship = world.try_get_component<Ship>(players.player1_id);
	if (!ship)
		return;
	auto& control = ship->control;
	control = ShipControl{};
	control.turn += input.get_action(action_turn_left) ? -1.f : 0.f;
	control.turn += input.get_action(action_turn_right) ? 1.f : 0.f;
	control.engine_burn_main += input.get_action(action_engine_burn) ? 1.f : 0.f;
	control.engine_burn_reverse += input.get_action(action_engine_reverse_burn) ? 1.f : 0.f;
	control.fire = input.get_action(action_fire);
}

void UpdatePhysicsClockSystem::run(const fr::FixedClock& fixed_clock, PhysicsClock& physics_clock) {
	physics_clock.tick(fixed_clock.delta());
}

void MovementSystem::run(World& world, const GameConsts& consts) {
	const auto ships = world.build_uncached_query<Body, const fr::Transform, const Ship>();
	ships.for_each([&](Body& body, const fr::Transform& transform, const Ship& ship) {
		body.force = {};
		body.angular_velocity = {};
		const auto ship_dir = unit_dir(transform.rotation);
		body.angular_velocity -= ship.control.turn * consts.ship.rotation_speed;
		body.force += ship_dir *
			(ship.control.engine_burn_main * consts.ship.engine_thrust
			- ship.control.engine_burn_reverse * consts.ship.engine_reverse_thrust);
	});
}

void UpdateGameClockSystem::run(const fr::AppClock& app_clock, GameClock& game_clock) {
	game_clock.tick(app_clock.delta());
}

void ShootingSystem::run(
	World& world,
	GameResources& resources,
	const GameConsts& consts,
	const PhysicsClock& clock
) {
	const auto query = world.build_uncached_query<
		Gun,
		const fr::Transform,
		const Body,
		const Ship
	>();
	query.for_each([&] (
		Gun& gun,
		const fr::Transform& transform,
		const Body& body,
		const Ship& ship
	) {
		gun.untill_reload -= clock.f_delta();
		if (ship.control.fire) {
			std::vector<BulletSpec> delayed_bullets;
			// The rate of fire can be higher than the fixed update rate. Subdivide into subticks
			const auto next_body = fr::euler_step(body.velocity, transform.position,
				body.force / body.mass, clock.f_delta().count());
			// NOTE: There is no `wrap_angle_neg_pi_pi`, it would cause interpolation issues
			// TODO: Write `lerp_angle` to properly handle two "wrapped" orientations
			const auto next_orientation = transform.rotation
				+ body.angular_velocity * clock.f_delta().count();

			for (; gun.untill_reload <= fr::FDuration::zero(); gun.untill_reload += gun.cooldown) {
				const auto inter = (gun.untill_reload + clock.f_delta()) / clock.f_delta();
				const auto inter_pos = glm::mix(transform.position, next_body.position, inter);
				const auto inter_velocity = glm::mix(body.velocity, next_body.velocity, inter);
				const auto inter_orientation = glm::mix(transform.rotation, next_orientation,
					inter);
				const auto inter_dir = unit_dir(inter_orientation);
				delayed_bullets.push_back({
					.pos = inter_pos + 0.5f * transform.scale.x * inter_dir, // ship's nose
					.rotation = inter_orientation,
					.velocity = inter_velocity + inter_dir * consts.bullet.speed,
					.step_delta = -gun.untill_reload
				});
			}
			for (const auto& bullet : delayed_bullets)
				make_bullet(world, resources, consts, bullet);
		}
		gun.untill_reload = std::max(fr::FDuration::zero(), gun.untill_reload);
	});
}

void PhysicsSystem::run(
	World& world,
	const WorldBounds& bounds,
	PhysicsClock& clock,
	fr::DebugDrawAdHoc& debug_draw
) {
	const auto process = [&](fr::Transform& transform, Body& body, fr::FDuration delta) {
		const auto dt = delta.count();
		const auto acceleration = body.force / body.mass;
		const auto next = fr::euler_step(body.velocity, transform.position, acceleration, dt);
		body.velocity = next.velocity;
		transform.position = next.position;

		transform.rotation += body.angular_velocity * dt;
		transform.rotation = wrap_angle_neg_pi_pi(transform.rotation);

		// Torus topology. Wraparound only if the object is not already flying into the bounds
		if (transform.position.x > bounds.value.max().x && body.velocity.x >= 0.f) {
			transform.position.x -= bounds.value.x_length();
		}
		else if (transform.position.x < bounds.value.min().x && body.velocity.x <= 0.f) {
			transform.position.x += bounds.value.x_length();
		}

		if (transform.position.y > bounds.value.max().y && body.velocity.y >= 0.f) {
			transform.position.y -= bounds.value.y_length();
		}
		else if (transform.position.y < bounds.value.min().y && body.velocity.y <= 0.f) {
			transform.position.y += bounds.value.y_length();
		}

		if (!fr::almost_zero_len(body.velocity, 2)) {
			debug_draw.add_to_system(fr::DebugLine::from_dir(transform.position,
				0.2f * body.velocity));
		}
		if (!fr::almost_zero_len(acceleration, 2)) {
			debug_draw.add_to_system(fr::DebugLine::from_dir(transform.position,
				0.2f * acceleration, {fr::color_red, 0.95f}));
		}
	};
	world.build_uncached_query<fr::Transform, Body>().for_each([&](fr::Transform& transform, Body& body) {
		process(transform, body, clock.f_delta());
	});
	// Ugly workaround of iterator invalidation
	auto substepped_entities = std::vector<std::pair<Entity, Body>>{};
	world.build_uncached_query<fr::Transform, BodySubstepped>()
		.for_each([&](Entity entity, fr::Transform& transform, BodySubstepped& body) {
			process(transform, body.body, body.step_delta);
			substepped_entities.emplace_back(entity, body.body);
		});
	for (const auto& [entity, body] : substepped_entities) {
		world.remove_component<BodySubstepped>(entity);
		world.emplace_component<Body>(entity, body);
	}
	debug_draw.add_to_system(fr::DebugRect{.bounds = bounds.value,
		.shape_fill = fr::ShapeFill::Wire});
}

void ParentTransformSystem::run(World& world) {
	const auto parents = world.build_uncached_query<fr::Transform,
		const fr::ParentRelationship<Entity>>();
	parents.for_each([&](
		fr::Transform& child_transform,
		fr::ParentRelationship<Entity> relationship
	) {
		if (!world.contains(relationship.parent))
			return;
		const auto* parent_transform = world.try_get_component<fr::Transform>(relationship.parent);
		if (!parent_transform) {
			FR_LOG_WARN_MSG("Parent without Transfrom component");
			return;
		}

		child_transform = fr::Transform{transformed_by(relationship.local_transform,
			*parent_transform), child_transform.scale};
	});
}

void BoundsSystem::run(World& world) {
	const auto bounds = world.build_uncached_query<fr::Collider, const fr::Transform>();
	bounds.for_each([&](fr::Collider& collider, const fr::Transform& transform) {
		collider.transform_to(transform);
	});
}

void CollisionSystem::run(
	World& world,
	CollisionData& collision_data,
	const fr::CollisionMatrix& collision_matrix
) {
	// TODO: Implement a native function to destroy entities matched by a query
	collision_data.collisions.clear();

	const auto colliders_query = world.build_uncached_query<const fr::Collider>();
	auto entities = std::vector<Entity>{};
	auto colliders = std::vector<fr::Collider>{};
	colliders_query.for_each([&](Entity eid, const fr::Collider& collider) {
		entities.push_back(eid);
		colliders.push_back(collider);
	});

	for (auto i = 0zu; i < colliders.size(); ++i) {
		for (auto j = i + 1zu; j < colliders.size(); ++j) {
			const auto& ca = colliders[i];
			const auto& cb = colliders[j];
			if (!collision_matrix.get(ca.layer(), cb.layer())) {
				continue;
			}
			if (colliders_intersect(ca, cb)) {
				const auto a = entities[i];
				const auto b = entities[j];
				collision_data.collisions.emplace_back(a, ca.layer(), b, cb.layer());
			}
		}
	}
}

static
void handle_asteroid_bullet_collision(
	World& world,
	GameScore& score,
	GameResources& resources,
	const GameConsts& consts,
	Prng& prng,
	Entity asteroid_id,
	Entity bullet_id
) {
	world.emplace_component<ToBeDestroyed>(bullet_id);
	const auto [asteroid, transform, body]
		= world.try_get_components<Asteroid, fr::Transform, Body>(asteroid_id);
	if (!asteroid || !transform || !body) {
		FR_LOG_ERROR_MSG("Collision resolution: missing asteroid components");
		return;
	}
	const auto* const damage_inflictor = world.try_get_component<DamageInflictor>(bullet_id);
	if (!damage_inflictor) {
		FR_LOG_ERROR_MSG("Collision resolution: missing bullet components");
		return;
	}

	if (asteroid->hp > damage_inflictor->damage) {
		asteroid->hp -= damage_inflictor->damage;
	}
	else {
		// Handle asteroid kill
		asteroid->hp = 0;
		score.value += consts.asteroid.levels[static_cast<size_t>(asteroid->level)].reward;
		if (asteroid->level > 0) {
			// Creating components invalidates references and pointers. To avoid crashes,
			// copy all the necessary component data beforehand
			const auto make_fragment = [&, aster = *asteroid, t = *transform, b = *body] {
				const auto phi = fr::FRad{fr::roll_uniform(prng.engine, 0.f, fr::f_tau)};
				const auto dir = unit_dir(phi);
				const auto velocity_shift = consts.asteroid.fragment_speed_variance * dir;
				// Spread asteroid fragments apart
				FR_ASSERT(aster.level <= consts.asteroid.max_level());
				const auto pos_shift
					= 0.5f * consts.asteroid.levels[size_t(aster.level)].radius * dir;
				const auto pos = t.position + pos_shift;
				make_asteroid(world, resources, consts, prng, aster.level - 1,
					pos, b.velocity + velocity_shift);
				FR_LOG_TRACE("Created asteroid fragment at ({} {})", pos.x, pos.y);
			};
			make_fragment();
			make_fragment();
		}
		world.emplace_component<ToBeDestroyed>(asteroid_id);
	}
}

void CollisionResolutionSystem::run(
	World& world,
	GameScore& score,
	GameResources& resources,
	const CollisionData& collision_data,
	const GameConsts& consts,
	Prng& prng,
	fr::MessageWriter<ReqEndGame>& msg_writer
) {

	for (const auto& col : collision_data.collisions) {
		FR_ASSERT(world.contains(col.first_entity()));
		FR_ASSERT(world.contains(col.second_entity()));
		if (world.has_component<ToBeDestroyed>(col.first_entity())
			|| world.has_component<ToBeDestroyed>(col.second_entity())
		) {
			return;
		}

		if (const auto m = col.match(collider_asteroid, collider_bullet); m) {
			handle_asteroid_bullet_collision(world, score, resources, consts, prng, m->first,
				m->second);
			return;
		}
		if (const auto m = col.match(collider_player, collider_asteroid); m) {
			msg_writer.push(ReqEndGame{});
			return;
		}
	};
}

void ExpirationSystem::run(World& world, const GameClock& clock) {
	const auto expirables = world.build_uncached_query<Expirable>();
	auto delayed = std::vector<Entity>{};
	expirables.for_each([&](Entity eid, Expirable& expirable) {
		expirable.time_to_live -= clock.f_delta();
		if (expirable.time_to_live <= fr::FDuration::zero()) {
			delayed.push_back(eid);
		}
	});
	for (auto eid : delayed)
		world.emplace_component<ToBeDestroyed>(eid);
}

void SpawnSystem::run(
	World& world,
	const WorldBounds& bounds,
	GameResources& resources,
	const GameConsts& consts,
	const GameClock& clock,
	Prng& prng
) {
	const auto world_perimeter = shape_perimeter(bounds.value);
	std::uniform_real_distribution<float> pos_dist {0.f, world_perimeter};
	const auto map_to_pos = [&](float val) {
		// Evenly distribute along the world edges
		if (val < 2.f * bounds.value.x_length()) {
			// Horizontal edge
			return val < bounds.value.x_length()
				? glm::vec2{bounds.value.min().x + val, bounds.value.min().y}
				: glm::vec2{bounds.value.min().x + (val - bounds.value.x_length()),
					bounds.value.max().y};
		}
		else {
			// Vertical edge
			const float grounded = val - 2.f * bounds.value.x_length();
			return grounded < bounds.value.y_length()
				? glm::vec2{bounds.value.min().x, bounds.value.min().y + grounded}
				: glm::vec2{bounds.value.max().x, bounds.value.min().y
					+ (grounded - bounds.value.y_length())};
		}
	};

	const auto spawners = world.build_uncached_query<AsteroidSpawner>();
	spawners.for_each([&](AsteroidSpawner& spawner) {
		spawner.untill_spawn -= clock.f_delta();
		if (spawner.untill_spawn <= fr::FDuration::zero()) {
			spawner.untill_spawn += spawner.cooldown;

			const auto rand_val = pos_dist(prng.engine);
			const auto pos = map_to_pos(rand_val);
			// Shoot towards some point in the center
			const auto world_bounds = bounds.value.padded(-0.15f * bounds.value.size());
			const auto target = fr::roll_uniform(prng.engine, world_bounds.min(),
				world_bounds.max());
			const auto velocity = consts.asteroid.speed * glm::normalize(target - pos);
			make_asteroid(world, resources, consts, prng, consts.asteroid.max_level(), pos,
				velocity);
		}
	});
}

void WorldCleanupSystem::run(World& world) {
	const auto to_be_destroyed = world.build_uncached_query<ToBeDestroyed>();
	auto delayed = std::vector<Entity>{};

	to_be_destroyed.for_each([&](Entity eid, ToBeDestroyed) {
		delayed.push_back(eid);
	});

	for (auto eid: delayed) {
		world.despawn(eid);
	}
}

} // namespace aster
