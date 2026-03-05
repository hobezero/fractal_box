#include <catch2/catch_test_macros.hpp>
#include <nanobench.h>

#include <array>
#include <span>

#include <fmt/format.h>

#include "fractal_box/runtime/world.hpp"

namespace nanobench = ankerl::nanobench;
using namespace std::string_view_literals;

namespace {

struct A {
	int64_t x;
	int64_t y;
};

struct B {
	std::string s;
};

struct C {
	constexpr
	C(
		float m0, float m1, float m2,
		float m3, float m4, float m5,
		float m6, float m7, float m8
	) noexcept:
		f{m0, m1, m2, m3, m4, m5, m6, m7, m8}
	{ }

	float f[9];
};

struct D {
	double x;
};

struct E {
	float x;
	float y;
	float z;
};

struct F {
	std::string s1;
	std::string s2;
};

struct G {
	int32_t a;
	int32_t b;
	double c;
};

struct H {
	float x;
	float y;
	float z;
	float w;
};

struct I {
	I(int16_t a0, int16_t a1, int16_t a2, int16_t a3) noexcept:
		a{a0, a1, a2, a3}
	{ }

public:
	std::array<int16_t, 4> a;
};

struct J {
	std::vector<int> v;
	int i;
};

struct Q { };

static constexpr auto bench_entity_count = 120'000zu;

static
auto make_bench_name(std::string_view name, bool is_warmup) -> std::string {
	if (is_warmup)
		return std::format("! WARMUP ! {}", name);
	else
		return std::string(name);
}

template<fr::c_world TWorld>
static
auto fill_world(
	TWorld& world,
	std::span<typename TWorld::Entity> entities,
	size_t num_comps_per_entity
) {
	auto rng = nanobench::Rng{1};

	for (auto& eid : entities)
		eid = world.spawn();

	for (auto i = 0zu ; i < num_comps_per_entity * entities.size(); ++i) {
		const auto eid = entities[rng.bounded(static_cast<uint32_t>(entities.size()))];
		switch (rng.bounded(5)) {
			case 0:
				world.template emplace_component<A>(eid, 24, 56);
				break;
			case 1:
				world.template emplace_component<B>(eid, "AVCASDASD");
				break;
			case 2:
				world.template emplace_component<C>(eid, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
				break;
			case 3:
				world.template emplace_component<D>(eid, 5.6);
				break;
			case 4:
				world.template emplace_component<E>(eid, 1.f, 2.f, 3.f);
				break;
		}
	}
}

static
auto bench_spawn(nanobench::Bench& bench, bool is_warmup = false) {
	auto world = fr::World<>{};
	bench.run(make_bench_name("spawn", is_warmup), [&] {
		nanobench::doNotOptimizeAway(world.spawn());
	});
}

static
auto bench_despawn(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[5zu * bench_entity_count];
	fill_world(world, entities, 3zu);

	bench.run(make_bench_name("despawn", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		nanobench::doNotOptimizeAway(world.despawn(eid));
	});
}

static
auto bench_try_get_component(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 5zu);

	bench.run(make_bench_name("try_get_component", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.try_get_component<A>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.try_get_component<B>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.try_get_component<C>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.try_get_component<D>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.try_get_component<E>(eid));
				break;
		}
	});
}

static
auto bench_try_get_components_one(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 5zu);

	bench.run(make_bench_name("try_get_components (one)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.try_get_components<A>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.try_get_components<B>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.try_get_components<C>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.try_get_components<D>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.try_get_components<E>(eid));
				break;
		}
	});
}

static
auto bench_try_get_components_two(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 5zu);

	bench.run(make_bench_name("try_get_components (two)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.try_get_components<A, D>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.try_get_components<B, E>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.try_get_components<C, B>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.try_get_components<D, C>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.try_get_components<E, A>(eid));
				break;
		}
	});
}

static
auto bench_emplace_component(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	for (auto& eid : entities)
		eid = world.spawn();

	bench.run(make_bench_name("emplace_component", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				world.emplace_component<A>(eid, 24, 56);
				break;
			case 1:
				world.emplace_component<B>(eid, "AVCASDASD");
				break;
			case 2:
				world.emplace_component<C>(eid, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
				break;
			case 3:
				world.emplace_component<D>(eid, 5.6);
				break;
			case 4:
				world.emplace_component<E>(eid, 1.f, 2.f, 3.f);
				break;
		}
	});
}

static
auto bench_emplace_components_one(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	for (auto& eid : entities)
		eid = world.spawn();

	bench.run(make_bench_name("emplace_components (one)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				world.emplace_components(eid, fr::in_place_args<A>(24, 56));
				break;
			case 1:
				world.emplace_components(eid, fr::in_place_args<B>("AVCASDASD"));
				break;
			case 2:
				world.emplace_components(eid, fr::in_place_args<C>(
					1.f, 2.f, 3.f,
					4.f, 5.f, 6.f,
					7.f, 8.f, 9.f
				));
				break;
			case 3:
				world.emplace_components(eid, fr::in_place_args<D>(5.6));
				break;
			case 4:
				world.emplace_components(eid, fr::in_place_args<E>(1.f, 2.f, 3.f));
				break;
		}
	});
}

static
auto bench_emplace_components_two(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	for (auto& eid : entities)
		eid = world.spawn();

	bench.run(make_bench_name("emplace_components (two)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				world.emplace_components(eid,
					fr::in_place_args<A>(24, 56),
					fr::in_place_args<C>(
						1.f, 2.f, 3.f,
						4.f, 5.f, 6.f,
						7.f, 8.f, 9.f
					)
				);
				break;
			case 1:
				world.emplace_components(eid,
					fr::in_place_args<B>("AVCASDASD"),
					fr::in_place_args<D>(5.6)
				);
				break;
			case 2:
				world.emplace_components(eid,
					fr::in_place_args<C>(
						1.f, 2.f, 3.f,
						4.f, 5.f, 6.f,
						7.f, 8.f, 9.f
					),
					fr::in_place_args<E>(1.f, 2.f, 3.f)
				);
				break;
			case 3:
				world.emplace_components(eid,
					fr::in_place_args<D>(5.6),
					fr::in_place_args<A>(24, 56)
				);
				break;
			case 4:
				world.emplace_components(eid,
					fr::in_place_args<E>(1.f, 2.f, 3.f),
					fr::in_place_args<B>("AVCASDASD")
				);
				break;
		}
	});
}

static
auto bench_remove_component(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 5zu);

	bench.run(make_bench_name("remove_component", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.remove_component<A>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.remove_component<B>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.remove_component<C>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.remove_component<D>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.remove_component<E>(eid));
				break;
		}
	});
}

static
auto bench_remove_components_one(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 6zu);

	bench.run(make_bench_name("remove_components (one)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.remove_components<A>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.remove_components<B>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.remove_components<C>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.remove_components<D>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.remove_components<E>(eid));
				break;
		}
	});
}

static
auto bench_remove_components_two(nanobench::Bench& bench, bool is_warmup = false) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[bench_entity_count];
	fill_world(world, entities, 6zu);

	bench.run(make_bench_name("remove_components (two)", is_warmup), [&] {
		const auto eid = entities[rng.bounded(std::size(entities))];
		switch (rng.bounded(5)) {
			case 0:
				nanobench::doNotOptimizeAway(world.remove_components<A, E>(eid));
				break;
			case 1:
				nanobench::doNotOptimizeAway(world.remove_components<B, D>(eid));
				break;
			case 2:
				nanobench::doNotOptimizeAway(world.remove_components<C, E>(eid));
				break;
			case 3:
				nanobench::doNotOptimizeAway(world.remove_components<D, C>(eid));
				break;
			case 4:
				nanobench::doNotOptimizeAway(world.remove_components<E, B>(eid));
				break;
		}
	});
}

template<fr::c_world TWorld>
static
auto access_rng_component(nanobench::Rng& rng, TWorld& world, typename TWorld::Entity eid) {
	switch (rng.bounded(10)) {
		case 0:
			nanobench::doNotOptimizeAway(world.template try_get_component<A>(eid));
			break;
		case 1:
			nanobench::doNotOptimizeAway(world.template try_get_component<B>(eid));
			break;
		case 2:
			nanobench::doNotOptimizeAway(world.template try_get_component<C>(eid));
			break;
		case 3:
			nanobench::doNotOptimizeAway(world.template try_get_component<D>(eid));
			break;
		case 4:
			nanobench::doNotOptimizeAway(world.template try_get_component<E>(eid));
			break;
		case 5:
			nanobench::doNotOptimizeAway(world.template try_get_component<F>(eid));
			break;
		case 6:
			nanobench::doNotOptimizeAway(world.template try_get_component<G>(eid));
			break;
		case 7:
			nanobench::doNotOptimizeAway(world.template try_get_component<H>(eid));
			break;
		case 8:
			nanobench::doNotOptimizeAway(world.template try_get_component<I>(eid));
			break;
		case 9:
			nanobench::doNotOptimizeAway(world.template try_get_component<J>(eid));
			break;
	}
}

template<fr::c_world TWorld>
static
auto emplace_rng_component(nanobench::Rng& rng, TWorld& world, typename TWorld::Entity eid) {
	switch (rng.bounded(11)) {
		case 0:
			world.template emplace_component<A>(eid, 24, 56);
			break;
		case 1:
			world.template emplace_component<B>(eid, "AVCASDASD");
			break;
		case 2:
			world.template emplace_component<C>(eid, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
			break;
		case 3:
			world.template emplace_component<D>(eid, 5.6);
			break;
		case 4:
			world.template emplace_component<E>(eid, 1.f, 2.f, 3.f);
			break;
		case 5:
			world.template emplace_component<F>(eid, "abcdef", "QWERTYUIOP{}|ASDFGHJKL:");
			break;
		case 6:
			world.template emplace_component<G>(eid, 61, 62, 6.3);
			break;
		case 7:
			world.template emplace_component<H>(eid, 7.f, 7.1f, 7.2f, 7.3f);
			break;
		case 8:
			world.template emplace_component<I>(eid, 81, 82, 83, 84);
			break;
		case 9:
			world.template emplace_component<J>(eid, std::vector<int>{91, 92, 93, 94}, 95);
			break;
		case 10:
			world.template emplace_component<Q>(eid);
			break;
	}
}

template<fr::c_world TWorld>
static
auto emplace_rng_components(nanobench::Rng& rng, TWorld& world, typename TWorld::Entity eid) {
	switch (rng.bounded(11)) {
		case 0:
			world.emplace_components(eid,
				fr::in_place_args<A>(24, 56),
				fr::in_place_args<B>("AVCASDASD")
			);
			break;
		case 1:
			world.emplace_components(eid,
				fr::in_place_args<B>("AVCASDASD"),
				fr::in_place_args<C>(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f)
			);
			break;
		case 2:
			world.emplace_components(eid,
				fr::in_place_args<C>(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f),
				fr::in_place_args<D>(5.6)
			);
			break;
		case 3:
			world.emplace_components(eid,
				fr::in_place_args<D>(5.6),
				fr::in_place_args<E>(1.f, 2.f, 3.f)
			);
			break;
		case 4:
			world.emplace_components(eid,
				fr::in_place_args<E>(1.f, 2.f, 3.f),
				fr::in_place_args<F>("abcdef", "QWERTYUIOP{}|ASDFGHJKL:")
			);
			break;
		case 5:
			world.emplace_components(eid,
				fr::in_place_args<F>("abcdef", "QWERTYUIOP{}|ASDFGHJKL:"),
				fr::in_place_args<G>(61, 62, 6.3)
			);
			break;
		case 6:
			world.emplace_components(eid,
				fr::in_place_args<G>(61, 62, 6.3),
				fr::in_place_args<H>(7.f, 7.1f, 7.2f, 7.3f)
			);
			break;
		case 7:
			world.emplace_components(eid,
				fr::in_place_args<H>(7.f, 7.1f, 7.2f, 7.3f),
				fr::in_place_args<Q>()
			);
			break;
		case 8:
			world.emplace_components(eid,
				fr::in_place_args<I>(81, 82, 83, 84),
				fr::in_place_args<J>(std::vector<int>{91, 92, 93, 94}, 95)
			);
			break;
		case 9:
			world.emplace_components(eid,
				fr::in_place_args<J>(std::vector<int>{91, 92, 93, 94}, 95),
				fr::in_place_args<A>(24, 56)
			);
			break;
		case 10:
			world.emplace_components(eid,
				fr::in_place_args<Q>(),
				fr::in_place_args<I>(81, 82, 83, 84)
			);
			break;
	}
}

template<fr::c_world TWorld>
static
auto remove_rng_component(nanobench::Rng& rng, TWorld& world, typename TWorld::Entity eid) {
	switch (rng.bounded(11)) {
		case 0:
			world.template remove_component<A>(eid);
			break;
		case 1:
			world.template remove_component<B>(eid);
			break;
		case 2:
			world.template remove_component<C>(eid);
			break;
		case 3:
			world.template remove_component<D>(eid);
			break;
		case 4:
			world.template remove_component<E>(eid);
			break;
		case 5:
			world.template remove_component<F>(eid);
			break;
		case 6:
			world.template remove_component<G>(eid);
			break;
		case 7:
			world.template remove_component<H>(eid);
			break;
		case 8:
			world.template remove_component<I>(eid);
			break;
		case 9:
			world.template remove_component<J>(eid);
			break;
		case 10:
			world.template remove_component<Q>(eid);
			break;
	}
}

template<fr::c_world TWorld>
static
auto remove_rng_components(nanobench::Rng& rng, TWorld& world, typename TWorld::Entity eid) {
	switch (rng.bounded(11)) {
		case 0:
			world.template remove_components<A, C>(eid);
			break;
		case 1:
			world.template remove_components<B, Q>(eid);
			break;
		case 2:
			world.template remove_components<C, E>(eid);
			break;
		case 3:
			world.template remove_components<D, F>(eid);
			break;
		case 4:
			world.template remove_components<E, G>(eid);
			break;
		case 5:
			world.template remove_components<F, H>(eid);
			break;
		case 6:
			world.template remove_components<G, I>(eid);
			break;
		case 7:
			world.template remove_components<H, J>(eid);
			break;
		case 8:
			world.template remove_components<I, A>(eid);
			break;
		case 9:
			world.template remove_components<J, B>(eid);
			break;
		case 10:
			world.template remove_components<Q, D>(eid);
			break;
	}
}

static
auto bench_world_stress(nanobench::Bench& bench) {
	using World = fr::World<>;
	auto rng = nanobench::Rng{0};
	auto world = World{};

	World::Entity entities[200'000];
	auto entity_count = 100'000zu;

	for (auto i = 0zu ; i < entity_count; ++i) {
		entities[i] = world.spawn();
	}

	for (auto i = 0zu ; i < 5 * entity_count; ++i) {
		const auto eid = entities[rng.bounded(static_cast<uint32_t>(entity_count))];
		emplace_rng_component(rng, world, eid);
	}

	bench.run("operation soup", [&] {
		if (entity_count == 0zu) {
			entities[0zu] = world.spawn();
			++entity_count;
			return;
		}
		auto& eid = entities[rng.bounded(static_cast<uint32_t>(entity_count))];
		switch (rng.bounded(9)) {
			case 0: case 1: case 2:
				access_rng_component(rng, world, eid);
				break;
			case 3:
				emplace_rng_component(rng, world, eid);
				break;
			case 4:
				emplace_rng_components(rng, world, eid);
				break;
			case 5:
				remove_rng_component(rng, world, eid);
				break;
			case 6:
				remove_rng_components(rng, world, eid);
				break;
			case 7: {
				if (entity_count < std::size(entities)) {
					entities[entity_count] = world.spawn();
					++entity_count;
				}
				break;
			}
			case 8: {
				world.despawn(eid);
				eid = std::exchange(entities[entity_count - 1zu], World::Entity{});
				--entity_count;
				break;
			}
		}
	});
}

} // namespace

TEST_CASE("bench:world.component-operations", "[b][engine][world]") {
	auto bench = nanobench::Bench{};
	bench.title("World component operations")
		.epochIterations(1'000)
		.epochs(300)
		.warmup(50'000)
		.relative(false)
		.performanceCounters(false)
	;

	bench_spawn(bench, true);
	bench_despawn(bench, true);
	bench_try_get_component(bench, true);
	bench_try_get_components_one(bench, true);
	bench_try_get_components_two(bench, true);
	bench_emplace_component(bench, true);
	bench_emplace_components_one(bench, true);
	bench_emplace_components_two(bench, true);
	bench_remove_component(bench, true);
	bench_remove_components_one(bench, true);
	bench_remove_components_two(bench, true);

	bench.performanceCounters(true);
	bench_spawn(bench);
	bench_despawn(bench);
	bench_try_get_component(bench);
	bench_try_get_components_one(bench);
	bench_try_get_components_two(bench);
	bench_emplace_component(bench);
	bench_emplace_components_one(bench);
	bench_emplace_components_two(bench);
	bench_remove_component(bench);
	bench_remove_components_one(bench);
	bench_remove_components_two(bench);

	REQUIRE_FALSE(bench.results().empty());
}

TEST_CASE("bench:world.stress-testing", "[b][engine][world]") {
	auto bench = nanobench::Bench{};
	bench.title("World stress testing")
		.minEpochIterations(300'000)
		.minEpochTime(std::chrono::milliseconds{100})
		.epochs(80)
		.warmup(50'000)
		.relative(false)
		.performanceCounters(true)
	;

	bench_world_stress(bench);

	REQUIRE_FALSE(bench.results().empty());
}

extern "C"
void fr_world_emplace_component(fr::World<>& world, fr::World<>::Entity eid) noexcept {
	world.emplace_component<A>(eid, 24, 56);
}

extern "C"
void fr_world_emplace_components(fr::World<>& world, fr::World<>::Entity eid) noexcept  {
	world.emplace_components(eid, fr::in_place_args<A>(24, 56));
}
