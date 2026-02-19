#include "fractal_box/components/collision.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h> // format_as breaks without this include for some reason

#include "fractal_box/core/enum_utils.hpp"

[[maybe_unused]] static constexpr auto layer1 = fr::CollisionLayer{1u};
[[maybe_unused]] static constexpr auto layer2 = fr::CollisionLayer{2u};
[[maybe_unused]] static constexpr auto layer3 = fr::CollisionLayer{3u};

TEST_CASE("Collider", "[u][engine][collision]") {
	SECTION("type properties") {
		CHECK(std::is_copy_constructible_v<fr::Collider>);
		CHECK(std::is_copy_assignable_v<fr::Collider>);

		CHECK(std::is_trivially_copy_constructible_v<fr::Collider>);
		CHECK(std::is_trivially_copyable_v<fr::Collider>);
		CHECK(std::is_trivially_copy_assignable_v<fr::Collider>);
		CHECK(std::is_trivially_move_constructible_v<fr::Collider>);
		CHECK(std::is_trivially_move_assignable_v<fr::Collider>);
		CHECK(std::is_trivially_destructible_v<fr::Collider>);
	}

	SECTION("holds alternative") {
		const auto c = fr::Collider{layer2, fr::FCircle{{5.f, -11.f}, 25.f}};
		CHECK(c.index() == fr::collider_index<fr::FCircle>);
		CHECK(c.layer() == layer2);
//		CHECK_FALSE(c.is<ColliderRect>());
		CHECK_FALSE(c.is<fr::FPoint2d>());
		CHECK(c.try_get<fr::FPoint2d>() == nullptr);
		REQUIRE(c.is<fr::FCircle>());
		CHECK(c.unchecked_get<fr::FCircle>().center() == glm::vec2{5.f, -11.f});
		CHECK(c.unchecked_get<fr::FCircle>().radius() == 25.f);
		REQUIRE(c.try_get<fr::FCircle>() != nullptr);
		CHECK(c.try_get<fr::FCircle>()->center() == glm::vec2{5.f, -11.f});
		CHECK(c.try_get<fr::FCircle>()->radius() == 25.f);
	}

	// Some tests will be unstable on the non IEEE 754 systems
	CHECK(std::numeric_limits<float>::is_iec559);

	SECTION("AABB vs AABB") {
		const auto box_a = fr::FAaRect{{1.f, 2.f}, {5.f, 10.f}};
		const auto a = fr::Collider{layer1, box_a};
		SECTION("overlap") {
			const auto box_b = fr::FAaRect{{-2.f, 1.f}, {3.f, 5.f}};
			const auto b = fr::Collider{layer1, box_b};
			CHECK(fr::shapes_intersect(box_a, box_b));
			CHECK(fr::shapes_intersect(box_b, box_a));
			CHECK(fr::colliders_intersect(a, b));
			CHECK(fr::colliders_intersect(b, a));
		}

		SECTION("inside") {
			const auto box_b = fr::FAaRect{{2.f, 3.f}, {4.f, 6.f}};
			const auto b = fr::Collider{layer1, box_b};
			CHECK(fr::shapes_intersect(box_a, box_b));
			CHECK(fr::shapes_intersect(box_b, box_a));
			CHECK(fr::colliders_intersect(a, b));
			CHECK(fr::colliders_intersect(b, a));
		}

		SECTION("separated") {
			const auto box_b = fr::FAaRect{{-3.f, -2.f}, {-1.f, -1.f}};
			const auto b = fr::Collider{layer1, box_b};
			CHECK_FALSE(fr::shapes_intersect(box_a, box_b));
			CHECK_FALSE(fr::shapes_intersect(box_b, box_a));
			CHECK_FALSE(fr::colliders_intersect(a, b));
			CHECK_FALSE(fr::colliders_intersect(b, a));
		}

		SECTION("touching") {
			const auto box_b = fr::FAaRect{{5.f, 4.f}, {8.f, 8.f}};
			const auto b = fr::Collider{layer1, box_b};
			CHECK(fr::shapes_intersect(box_a, box_b));
			CHECK(fr::shapes_intersect(box_b, box_a));
			CHECK(fr::colliders_intersect(a, b));
			CHECK(fr::colliders_intersect(b, a));
		}
	}

	SECTION("Point vs Circle") {
		const auto circleA = fr::FCircle{{8.f, 6.f}, 3.f};
		const auto a = fr::Collider{layer1, circleA};

		SECTION("outside") {
			const auto pointB = fr::FPoint2d{5.f, 8.f};
			const auto b = fr::Collider{layer1, pointB};
			CHECK_FALSE(fr::shapes_intersect(pointB, circleA));
			CHECK_FALSE(fr::colliders_intersect(a, b));
			CHECK_FALSE(fr::colliders_intersect(b, a));
		}

		SECTION("inside") {
			const auto pointB = fr::FPoint2d{7.f, 4.f};
			const auto b = fr::Collider{layer1, pointB};
			CHECK(fr::shapes_intersect(pointB, circleA));
			CHECK(fr::colliders_intersect(a, b));
			CHECK(fr::colliders_intersect(b, a));
		}

		// TODO: user powers of two to avoid floating-point issues
		SECTION("touching") {
			const auto pointB = fr::FPoint2d{5.f, 6.f};
			const auto b = fr::Collider{layer1, pointB};
			CHECK(fr::shapes_intersect(pointB, circleA));
			CHECK(fr::colliders_intersect(a, b));
			CHECK(fr::colliders_intersect(b, a));
		}
	}

	SECTION("Point vs Point") {
		const auto pointA = fr::FPoint2d{-5.f, 2.f};
		const auto a = fr::Collider{layer1, pointA};

		SECTION("shapes_intersect") {
			CHECK(fr::shapes_intersect(pointA, pointA));
			CHECK(fr::colliders_intersect(a, a));
		}

		SECTION("separated") {
			const auto pointB = fr::FPoint2d{10.f, 7.f};
			const auto b = fr::Collider{layer1, pointB};
			CHECK_FALSE(fr::shapes_intersect(pointA, pointB));
			CHECK_FALSE(fr::colliders_intersect(a, b));
			CHECK_FALSE(fr::colliders_intersect(b, a));
		}
	}

	SECTION("Circle vs Circle") {
		const auto circle_a = fr::FCircle{{2.f, 1.f}, 3.f};
		const auto coll_a = fr::Collider{layer1, circle_a};

		SECTION("overlapped") {
			const auto circle_b = fr::FCircle{{-3.f, -2.f}, 4.f};
			const auto coll_b = fr::Collider{layer1, circle_b};
			CHECK(fr::shapes_intersect(circle_a, circle_b));
			CHECK(fr::colliders_intersect(coll_a, coll_b));
			CHECK(fr::colliders_intersect(coll_b, coll_a));
		}

		SECTION("separated") {
			const auto circle_b = fr::FCircle{{-7.f, 3.f}, 1.f};
			const auto coll_b = fr::Collider{layer1, circle_b};
			CHECK_FALSE(fr::shapes_intersect(circle_a, circle_b));
			CHECK_FALSE(fr::colliders_intersect(coll_a, coll_b));
			CHECK_FALSE(fr::colliders_intersect(coll_b, coll_a));
		}

		SECTION("touching") {
			const auto circle_b = fr::FCircle{{1.f, 5.f}, 1.f};
			const auto coll_b = fr::Collider{layer1, circle_b};
			const auto circle_c = fr::FCircle{{3.f, 5.f}, 1.f};
			const auto coll_c = fr::Collider{layer1, circle_c};
			CHECK(fr::shapes_intersect(circle_b, circle_c));
			CHECK(fr::colliders_intersect(coll_b, coll_c));
			CHECK(fr::colliders_intersect(coll_c, coll_b));
		}

		SECTION("inside") {
			const auto circle_b = fr::FCircle{{1.5f, 1.5f}, 1.f};
			const auto coll_b = fr::Collider{layer1, circle_b};
			CHECK(fr::shapes_intersect(circle_a, circle_b));
			CHECK(fr::colliders_intersect(coll_a, coll_b));
			CHECK(fr::colliders_intersect(coll_b, coll_a));
		}
	}

	// TODO: Point vs AABB
	// TODO: Circle vs AABB
}

TEST_CASE("Collision", "[u][engine][collision]") {
	SECTION("special member functions") {
		// TODO
	}

	SECTION("type properties") {
		// TODO
	}

	const auto entity_a = fr::EntityId32{5, 0};
	const auto entity_b = fr::EntityId32{7, 0};

	SECTION("initialization") {
		SECTION("collision layers in ascending order") {
FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_UNREACHABLE_CODE
			const auto collision = fr::Collision<fr::EntityId32>{entity_a, layer1, entity_b,
				layer2};
			if (collision.first_entity() == entity_a) {
				CHECK(collision.second_entity() == entity_b);
				CHECK(collision.first_layer() == layer1);
				CHECK(collision.second_layer() == layer2);
			}
			else if (collision.first_entity() == entity_b) {
				CHECK(collision.second_entity() == entity_a);
				CHECK(collision.first_layer() == layer2);
				CHECK(collision.second_layer() == layer1);
			}
			else {
				FAIL(fmt::format("Unexpected first_entity: {}", collision.first_entity()));
			}
FR_DIAGNOSTIC_POP
		}
		SECTION("collision layers in descending order") {
			const auto collision = fr::Collision<fr::EntityId32>{entity_a, layer3, entity_b,
				layer2};
			if (collision.first_entity() == entity_a) {
				CHECK(collision.second_entity() == entity_b);
				CHECK(collision.first_layer() == layer3);
				CHECK(collision.second_layer() == layer2);
			}
			else if (collision.first_entity() == entity_b){
				CHECK(collision.second_entity() == entity_a);
				CHECK(collision.first_layer() == layer2);
				CHECK(collision.second_layer() == layer3);
			}
			else {
				FAIL(fmt::format("Unexpected first_entity: {}", collision.first_entity()));
			}
		}
	}
	SECTION("matching") {
		SECTION("collision layers in ascending order") {
			const auto collision = fr::Collision<fr::EntityId32>{entity_a, layer1, entity_b,
				layer2};
			SECTION("match in initial order") {
				const auto match = collision.match(layer1, layer2);
				REQUIRE(match);
				CHECK(match->first == entity_a);
				CHECK(match->second == entity_b);
			}
			SECTION("match in reverse order") {
				const auto match = collision.match(layer2, layer1);
				REQUIRE(match);
				CHECK(match->first == entity_b);
				CHECK(match->second == entity_a);
			}
		}
		SECTION("collision layers in descending order") {
			const auto collision = fr::Collision<fr::EntityId32>{entity_a, layer3, entity_b,
				layer2};
			SECTION("match in initial order") {
				const auto match = collision.match(layer3, layer2);
				REQUIRE(match);
				CHECK(match->first == entity_a);
				CHECK(match->second == entity_b);
			}
			SECTION("match in reverse order") {
				const auto match = collision.match(layer2, layer3);
				REQUIRE(match);
				CHECK(match->first == entity_b);
				CHECK(match->second == entity_a);
			}
		}
	}
}
