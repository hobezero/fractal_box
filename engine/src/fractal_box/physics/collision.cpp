#include "fractal_box/physics/collision.hpp"

#include <array>
#include <limits>

#include "fractal_box/core/platform.hpp"
#include "fractal_box/math/math.hpp"

namespace fr {

static_assert(std::numeric_limits<float>::is_iec559);

static constexpr auto size = mp_size<ColliderTypeList>;

using Func = bool (*)(const Collider&, const Collider&);
using Table = std::array<std::array<Func, size>, size>;

template<class T, class U>
concept have_intersect_func = requires(T a, U b) {
	{ shapes_intersect(a, b) } -> std::same_as<bool>;
};

static_assert(have_intersect_func<FAaRect, FAaRect>);
static_assert(have_intersect_func<FPoint2d, FCircle>);
static_assert(have_intersect_func<FCircle, FPoint2d>);
static_assert(!have_intersect_func<FAaRect, ColliderRect>);
static_assert(!have_intersect_func<int, ColliderRect>);

template<class FirstCollider, class SecondCollider>
requires have_intersect_func<FirstCollider, SecondCollider>
static constexpr
void fill_cell(Table& table) noexcept {
	const auto first_idx = collider_index<FirstCollider>;
	const auto second_idx = collider_index<SecondCollider>;

	table[first_idx][second_idx] = +[](const Collider& a, const Collider& b) FR_FLATTEN {
		return shapes_intersect(
			a.unchecked_get<FirstCollider>(),
			b.unchecked_get<SecondCollider>()
		);
	};
}

/// @brief Fallback: do nothing
template<class FirstCollider, class SecondCollider>
static constexpr void fill_cell(...) noexcept { }

template<class T, class... Ts>
static constexpr
void fill_row(MpList<Ts...>, Table& table) noexcept {
	((fill_cell<T, Ts>(table)), ...);
}

template<class... Ts>
static constexpr
void fill_table(MpList<Ts...> list, Table& table) noexcept {
	((fill_row<Ts>(list, table)), ...);
}

static constexpr
auto make_table() noexcept -> Table {
	auto table = Table{};
	fill_table(collider_type_list, table);
	return table;
}

static constexpr
auto is_table_valid(const Table& table) noexcept -> bool {
	for (const auto& row : table) {
		for (const auto& cell : row) {
			if (cell == nullptr) {
				return false;
			}
		}
	}
	return true;
}

static constexpr auto table = make_table();
static_assert(is_table_valid(table));

// As a compromise, we give up constexprness of the table and initialize it at runtime from
// a constexpr variable defined in the .cpp file.
// It won't be used to initialize other static variables anyway, so SIOF can't bite us.
// Alternative: initialize the table manually in the header
const Collider::DispatcherTable Collider::dispatcher_table = table;

namespace experimental {

auto colliders_intersect_visit(const Collider& a, const Collider& b) noexcept -> bool {
	return a.visit([&b]<class A>(const A& a_spape) {
		return b.visit([a_spape]<class B>(const B& b_shape) {
			if constexpr (have_intersect_func<A, B>) {
				return shapes_intersect(a_spape, b_shape);
			}
			else {
				return false;
			}
		});
	});
}

auto colliders_intersect_switch(const Collider& a, const Collider& b) noexcept -> bool {
	switch (a.index()) {
		case collider_index<FPoint2d>: {
			const auto& a_shape = a.unchecked_get<FPoint2d>();
			switch (b.index()) {
				case collider_index<FPoint2d>:
					return shapes_intersect(a_shape, b.unchecked_get<FPoint2d>());
				case collider_index<FAaRect>:
					return shapes_intersect(a_shape, b.unchecked_get<FAaRect>());
				case collider_index<FCircle>:
					return shapes_intersect(a_shape, b.unchecked_get<FCircle>());
			}
			FR_UNREACHABLE();
			break;
		}
		case collider_index<FAaRect>: {
			const auto& iA = a.unchecked_get<FAaRect>();
			switch (b.index()) {
				case collider_index<FPoint2d>:
					return shapes_intersect(b.unchecked_get<FPoint2d>(), iA);
				case collider_index<FAaRect>:
					return shapes_intersect(iA, b.unchecked_get<FAaRect>());
				case collider_index<FCircle>:
					return shapes_intersect(b.unchecked_get<FCircle>(), iA);
			}
			FR_UNREACHABLE();
			break;
		}
		case collider_index<FCircle>: {
			const auto& iA = a.unchecked_get<FCircle>();
			switch (b.index()) {
				case collider_index<FPoint2d>:
					return shapes_intersect(b.unchecked_get<FPoint2d>(), iA);
				case collider_index<FAaRect>:
					return shapes_intersect(iA, b.unchecked_get<FAaRect>());
				case collider_index<FCircle>:
					return shapes_intersect(iA, b.unchecked_get<FCircle>());
			}
			FR_UNREACHABLE();
			break;
		}
	}
	FR_UNREACHABLE();
}

} // namespace experimental

auto Collider::center() const noexcept -> FPoint2d {
	switch (index()) {
		case collider_index<FPoint2d>: return unchecked_get<FPoint2d>();
		case collider_index<FAaRect>: return unchecked_get<FAaRect>().center();
		case collider_index<FCircle>: return unchecked_get<FCircle>().center();
	}
	FR_UNREACHABLE();
}

void Collider::transform_to(const Transform& transform) noexcept {
	switch (index()) {
		case collider_index<FPoint2d>:
			unchecked_get<FPoint2d>() = transform.position;
			break;
		case collider_index<FAaRect>:
			// transform.scale doesn't affect collider size
	unchecked_get<FAaRect>() = calc_aabb(transform.make_trans_rotation_matrix());
			break;
		case collider_index<FCircle>: {
			auto& circle = unchecked_get<FCircle>();
			circle.set_center(transform.position);
			// transform.scale doesn't affect collider radius
			break;
		}
	}
}

auto calc_aabb(const glm::mat3& transform) noexcept -> FAaRect {
	static constexpr glm::vec3 base_points[] {
		{-0.5f, -0.5f, 1.f},
		{-0.5f,  0.5f, 1.f},
		{ 0.5f,  0.5f, 1.f},
		{ 0.5f, -0.5f, 1.f},
	};

	auto aabb_min = FPoint2d{f_pos_inf, f_pos_inf};
	auto aabb_max = FPoint2d{f_neg_inf, f_neg_inf};
	for (const auto& point : base_points) {
		const glm::vec3 homo = transform * point;
		const auto transformed = glm::vec2{homo.x, homo.y};
		if (transformed.x < aabb_min.x) aabb_min.x = transformed.x;
		if (transformed.y < aabb_min.y) aabb_min.y = transformed.y;
		if (transformed.x > aabb_max.x) aabb_max.x = transformed.x;
		if (transformed.y > aabb_max.y) aabb_max.y = transformed.y;
	}
	return {aabb_min, aabb_max};
}

} // namespace fr
