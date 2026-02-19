#include <ranges>
#include <algorithm>

#include <catch2/catch_test_macros.hpp>
#include <nanobench.h>
#include <fmt/format.h>
#include <fmt/printf.h>

#include "fractal_box/core/random.hpp"
#include "fractal_box/components/collision.hpp"

namespace nanobench = ankerl::nanobench;

constexpr auto layer1 = fr::CollisionLayer{1u};
constexpr auto layer2 = fr::CollisionLayer{2u};

// TODO: Should we  use normal distribution
/// Make random Aabb
template<class Prng>
static
auto roll_aabb(Prng& prng, fr::FAaRect bounds, fr::FAaRect size_limits) -> fr::FAaRect {
	return {
		fr::from_center,
		fr::roll_uniform(prng, bounds.padded(-0.5f * size_limits.max())),
		fr::roll_uniform(prng, fr::FAaRect{0.5f * size_limits.min(), 0.5f * size_limits.max()})
	};
}

/// Make random Circle
template<class Prng>
static
auto roll_circle(
	Prng& prng,
	fr::FAaRect bounds,
	float min_radius,
	float max_radius
) -> fr::FCircle {
	return {
		fr::roll_uniform(prng, bounds.padded(-0.5f * glm::vec2{max_radius, max_radius})),
		fr::roll_uniform(prng, min_radius, max_radius)
	};
}

static
auto make_homogenous_samples(size_t sample_count, auto&& generator) {
	auto samples = std::vector<std::decay_t<decltype(generator())>>{};
	samples.reserve(sample_count);
	std::generate_n(std::back_inserter(samples), sample_count, generator);
	return samples;
}

namespace fr {
namespace {

struct NoopCollider {
	bool v{};
};

// It has to be in the fr namespace to be findable
auto shapes_intersect(NoopCollider a, NoopCollider b) noexcept -> bool {
	return (static_cast<int>(a.v) & static_cast<int>(b.v)) != 0;
}

} // namespace
} // namespace fr

TEST_CASE("bench:intesect.single-collision", "[b][collision]") {
	nanobench::Bench bench;
	bench.title("intersect(..) method")
		.performanceCounters(true)
		.relative(true)
		.warmup(100)
		.minEpochIterations(10'000);

	const auto bench_shapes = [&bench](const char* name, const auto& a, const auto& b) {
		// TODO: capture by value?
		bench.run(name, [&a, &b] {
			nanobench::doNotOptimizeAway(shapes_intersect(a, b));
		});
	};
	const auto bench_colliders = [&bench](const char* name, const auto& a, const auto& b) {
		// TODO: capture by value?
		bench.run(name, [&a, &b] {
			nanobench::doNotOptimizeAway(colliders_intersect(a, b));
		});
	};

	{ // Aabb vs Aabb
		namespace exper = ::fr::experimental;

		const auto box_a = fr::FAaRect{{1.f, 2.f}, {5.f, 10.f}};
		const auto box_b = fr::FAaRect{{-2.f, 1.f}, {3.f, 5.f}};

		const auto coll_box_a = fr::Collider{layer1, box_a};
		const auto coll_box_b = fr::Collider{layer1, box_b};

		bench.run("Collider[Aabb] vs Collider[Aabb] known", [&] {
			nanobench::doNotOptimizeAway(exper::colliders_intersect_aabb(coll_box_a, coll_box_b));
		});

		bench.run("Aabb vs Aabb", [&] {
			nanobench::doNotOptimizeAway(shapes_intersect(box_a, box_b));
		});

		bench.run("Collider[Aabb] vs Collider[Aabb]", [&] {
			nanobench::doNotOptimizeAway(colliders_intersect(coll_box_a, coll_box_b));
		});

		{ // Colliders that don't intersect
			const auto box_c = fr::FAaRect{{-3.f, -2.f}, {-1.f, -1.f}};
			const auto coll_box_c = fr::Collider{layer1, box_c};
			bench_colliders("Collider[Aabb] vs Collider[Aabb] false", coll_box_a, coll_box_c);
		}

		bench.run("Collider[Aabb] vs Collider[Aabb] Visit", [&] {
			nanobench::doNotOptimizeAway(exper::colliders_intersect_visit(coll_box_a, coll_box_b));
		});

		bench.run("Collider[Aabb] vs Collider[Aabb] Switch", [&] {
			nanobench::doNotOptimizeAway(exper::colliders_intersect_switch(coll_box_a, coll_box_b));
		});
	}
	{ // Point vs Circle
		const auto point_a = fr::FPoint2d{7.f, 4.f};
		const auto circle_b = fr::FCircle{{8.f, 6.f}, 3.f};

		const auto coll_point_a = fr::Collider{layer1, point_a};
		const auto coll_circle_b = fr::Collider{layer1, circle_b};

		bench_shapes("Point vs Circle", point_a, circle_b);
		bench_colliders("Collider[Point] vs Collider[Circle]", coll_point_a, coll_circle_b);
	}

	{ // Point vs Aabb
		const auto point_a = fr::FPoint2d{7.f, 4.f};
		const auto box_b = fr::FAaRect{{2.f, 3.f}, {9.f, 7.f}};

		const auto coll_point_a = fr::Collider{layer1, point_a};
		const auto coll_box_b = fr::Collider{layer1, box_b};

		bench_shapes("Point vs Aabb", point_a, box_b);
		bench_colliders("Collider[Point] vs Collider[Aabb]", coll_point_a, coll_box_b);
	}

	{ // Circle vs Circle
		const auto circle_a = fr::FCircle{{2.f, 1.f}, 3.f};
		const auto circle_b = fr::FCircle{{-3.f, -2.f}, 4.f};

		const auto coll_circle_a = fr::Collider{layer1, circle_a};
		const auto coll_circle_b = fr::Collider{layer1, circle_b};

		bench_shapes("Circle vs Circle", circle_a, circle_b);
		bench_colliders("Collider[Circle] vs Collider[Circle]", coll_circle_a, coll_circle_b);
	}

	{ // Circle vs Aabb
		const auto circleA = fr::FCircle{{2.f, 1.f}, 3.f};
		const auto box_b = fr::FAaRect{{-3.f, -2.f}, {1.f, 2.f}};

		const auto coll_circle_a = fr::Collider{layer1, circleA};
		const auto coll_box_b = fr::Collider{layer1, box_b};

		bench_shapes("Circle vs Aabb", circleA, box_b);
		bench_colliders("Collider[Circle] vs Collider[Aabb]", coll_circle_a, coll_box_b);
	}
	REQUIRE(!bench.results().empty());
}

TEST_CASE("bench:intesect.collision-batches", "[b][collision]") {
	nanobench::Bench bench;
	bench.title("intersect(..) batch method")
		.performanceCounters(true)
		.relative(true)
		.warmup(100)
		.minEpochIterations(1000);

	// RNG should be deterministic
	auto rng = nanobench::Rng(42ul);
	const auto sample_count = 10000uz;

	const auto bench_intersect_batch = [&bench, sample_count]<class A, class B>(
		std::string_view name_a, std::string_view nameB,
		const std::vector<A>& vec_a, const std::vector<B>& vec_b
	) -> int {
		// alignas to mitigate inconsistencies caused by cache line peculiarities
		struct alignas(32) Item { A a; B b; };

		std::vector<Item> test_data;
		test_data.reserve(sample_count);
		FR_ASSERT_CHECK(sample_count <= vec_a.size());
		FR_ASSERT_CHECK(sample_count <= vec_b.size());
		for (auto i = 0uz; i < sample_count; ++i) {
			// Shift the second index so that when vecA == vecB colliders won't itersect only
			// with themselves
			const auto bidx = (i + std::max(2ul, sample_count / 3ul)) % sample_count;
			test_data.push_back({vec_a[i], vec_b[bidx]});
		}

		int collision_count = -1;
		bench.run(fmt::format("x{} {} vs {}", sample_count, name_a, nameB),
			[&test_data, &collision_count] {
				collision_count = 0;
				for (const auto& item : test_data) {
					if constexpr (std::is_same_v<A, fr::Collider>
						&& std::is_same_v<B, fr::Collider>
					) {
						collision_count += colliders_intersect(item.a, item.b);
					}
					else {
						collision_count += shapes_intersect(item.a, item.b);
					}
				}
			}
		);
		nanobench::doNotOptimizeAway(collision_count);
		return collision_count;
	};

	const auto bench_intersect_wrapped = [&]<class A, class B>(
		std::string_view name_a, std::string_view name_b,
		const std::vector<A>& vec_a, const std::vector<B>& vec_b
	) {
		const auto count_plain = bench_intersect_batch(name_a, name_b, vec_a, vec_b);

		if constexpr (fr::mp_contains<fr::ColliderTypeList, A>
			&& fr::mp_contains<fr::ColliderTypeList, B>
		) {
			// Wrap shapes into colliders, then benchmark intersection again
			auto as_colliders = [](const auto& v) {
				return std::views::transform(v, [](const auto& c) {
					return fr::Collider{layer1, c};
				});
			};

			auto coll_view_a = as_colliders(vec_a);
			auto coll_view_b = as_colliders(vec_b);

			const auto count_wrapped = bench_intersect_batch(
				fmt::format("Collider[{}]", name_a), fmt::format("Collider[{}]", name_b),
				std::vector<fr::Collider>{coll_view_a.begin(), coll_view_a.end()},
				std::vector<fr::Collider>{coll_view_b.begin(), coll_view_b.end()}
			);
			CHECK(count_plain == count_wrapped);
		}
		return count_plain;
	};

	const auto world_bounds = fr::FAaRect{{-100.f, -100.f}, {100.f ,100.f}};
	const std::vector<glm::vec2> points = make_homogenous_samples(sample_count, [&] {
		return fr::roll_uniform(rng, world_bounds.min(), world_bounds.max());
	});
	const std::vector<fr::FAaRect> boxes = make_homogenous_samples(sample_count, [&] {
		return roll_aabb(rng, world_bounds, {{0.6f, 0.6f}, {69.2f, 69.2f}});
	});
	const std::vector<fr::FAaRect> large_boxes = make_homogenous_samples(sample_count, [&] {
		return roll_aabb(rng, world_bounds, {{10.f, 10.f}, {167.4f, 167.4f}});
	});
	const std::vector<fr::FCircle> circles = make_homogenous_samples(sample_count, [&] {
		return roll_circle(rng, world_bounds, 0.4f, 43.2f);
	});
	const std::vector<fr::FCircle> large_circles = make_homogenous_samples(sample_count, [&] {
		return roll_circle(rng, world_bounds, 15.f, 80.f);
	});
	const std::vector<fr::NoopCollider> noops = make_homogenous_samples(sample_count, [&] {
		return fr::NoopCollider{fr::flip_coin(rng)};
	});
	// large_circles and large_boxes are used to keep the fraction of collisions somewhat
	// consistent (around 20%)
	const auto deviation = [] (int x, double expected) {
		fmt::print("Hit count: {}\n", x);
		return std::abs(expected - static_cast<double>(x) / static_cast<double>(sample_count));
	};
	CHECK(deviation(bench_intersect_wrapped("Aabb", "Aabb", boxes, boxes), 0.2) < 0.05);
	CHECK(deviation(bench_intersect_wrapped("Point", "Circle", points, large_circles), 0.2) < 0.05);
	CHECK(deviation(bench_intersect_wrapped("Point", "Aabb", points, large_boxes), 0.2) < 0.05);
	CHECK(deviation(bench_intersect_wrapped("Circle", "Circle", circles, circles), 0.2) < 0.05);
	CHECK(deviation(bench_intersect_wrapped("Circle", "Aabb", circles, boxes), 0.2) < 0.05);

	{ // mix all collider types
		// Since it's just test code, it's acceptable to do extra work by copying data around
		std::vector<fr::Collider> colliders;
		colliders.reserve(sample_count);
		auto points_idx = 0uz;
		auto boxes_idx = 0uz;
		auto circles_idx = 0uz;
		for (auto i = 0uz; i < sample_count; ++i) {
			switch (i % fr::mp_size<fr::ColliderTypeList>) {
				case 0: colliders.push_back({layer1, points[points_idx++]}); break;
				case 1: colliders.push_back({layer1, boxes[boxes_idx++]}); break;
				case 2: colliders.push_back({layer1, circles[circles_idx++]}); break;
			}
		}
		std::shuffle(colliders.begin(), colliders.end(), rng);
		// TODO: Fix this test
		CHECK(deviation(bench_intersect_wrapped("Collider[random]", "Collider[random]",
			colliders, colliders), 0.2) < 0.05);
	}
	CHECK(deviation(bench_intersect_wrapped("noop", "noop", noops, noops), 0.2) < 0.05);

	REQUIRE(!bench.results().empty());
}

TEST_CASE("bench-CollisionMatrix", "[b][collision]") {
	auto bench = nanobench::Bench{};
	bench.title("collision matrix operation")
		.performanceCounters(true)
		.relative(true)
		.warmup(100)
		.minEpochIterations(20'000);

	auto matrix = fr::CollisionMatrix{};
	matrix.add(layer1, layer2);
	bench.run("CollisionMatrix::get", [&] {
		const auto r = matrix.get(layer1, layer2);
		nanobench::doNotOptimizeAway(r);
	});

	REQUIRE(!bench.results().empty());
}
