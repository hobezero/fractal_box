#ifndef FRACTAL_BOX_PHYSICS_COLLISION_HPP
#define FRACTAL_BOX_PHYSICS_COLLISION_HPP

#include <array>
#include <optional>
#include <variant>

#include <glm/vec2.hpp>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/math/shape_math.hpp"
#include "fractal_box/math/shapes.hpp"
#include "fractal_box/runtime/world_types.hpp"
#include "fractal_box/scene/transform.hpp"

namespace fr {

struct ColliderRect {
	glm::vec2 vertices[4];
};

enum class CollisionLayer: uint8_t { };
inline constexpr auto num_collision_layers = 20uz;
inline constexpr auto max_collision_layer = static_cast<CollisionLayer>(num_collision_layers - 1uz);

FR_FORCE_INLINE constexpr
auto is_collision_layer_valid(CollisionLayer layer) noexcept -> bool {
	return to_underlying(layer) <= to_underlying(max_collision_layer);
}

// TODO: ColliderRect
// TODO: Should we replace std::variant witih a custom union/std::aligned_union?
class Collider {
public:
	using Types = std::variant<
		FPoint2d,
		FAaRect,
		FCircle
	>;

	template<class T>
	Collider(CollisionLayer layer, T&& t):
		_layer{layer},
		_data{std::forward<T>(t)}
	{ }

	/// @pre this->is<T>()
	template<class T>
	auto unchecked_get() noexcept -> T& {
		// Avoid branching by telling compiler there is only one option.
		// Unfortunately, __builtin_assume() produces a false-positive warning
		// (the argument has side effects that will be discarded)
		if (!std::holds_alternative<T>(_data)) {
			FR_UNREACHABLE();
		}
		return std::get<T>(_data);
	}

	/// @pre this->is<T>()
	template<class T>
	auto unchecked_get() const noexcept -> const T& {
		// TODO: Replace with `FR_ASSUME`
		if (!std::holds_alternative<T>(_data)) {
			FR_UNREACHABLE();
		}
		return std::get<T>(_data);
	}

	template<class T>
	auto try_get() noexcept -> T* { return std::get_if<T>(&_data); }

	template<class T>
	auto try_get() const noexcept -> const T* { return std::get_if<T>(&_data); }

	template<class T>
	auto is() const noexcept -> bool { return std::holds_alternative<T>(_data); }

	auto index() const noexcept -> size_t { return _data.index(); }
	auto layer() const noexcept -> CollisionLayer { return _layer; }

	auto center() const noexcept -> FPoint2d;
	void transform_to(const Transform& transform) noexcept;

	template<class Visitor>
	auto visit(Visitor&& v) -> decltype(auto) {
		return std::visit(std::forward<Visitor>(v), _data);
	}

	template<class Visitor>
	auto visit(Visitor&& v) const -> decltype(auto) {
		return std::visit(std::forward<Visitor>(v), _data);
	}

private:
	using DispatcherFunc = bool (*)(const Collider&, const Collider&);
	using DispatcherTable = std::array<std::array<DispatcherFunc, std::variant_size_v<Types>>,
		std::variant_size_v<Types>>;
	static const DispatcherTable dispatcher_table;

	friend
	auto colliders_intersect(const Collider& a, const Collider& b) noexcept -> bool;

private:
	CollisionLayer _layer {};
	Types _data;
};

inline
auto colliders_intersect(const Collider& a, const Collider& b) noexcept -> bool {
	const auto func = Collider::dispatcher_table[a.index()][b.index()];
	return func(a, b);
}

using ColliderTypeList = ToMpList<Collider::Types>;
inline constexpr ColliderTypeList collider_type_list {};

template<class T>
using ColliderIndex = MpFind<T, ColliderTypeList>;

template<class T>
inline constexpr auto collider_index = mp_find<ColliderTypeList, T>;

class CollisionMatrix {
	using UnderlyingT = std::underlying_type_t<CollisionLayer>;
	static_assert(std::is_unsigned_v<UnderlyingT>);

public:
	constexpr
	auto get(CollisionLayer lhs, CollisionLayer rhs) const noexcept -> bool {
		FR_ASSERT_MSG(is_collision_layer_valid(lhs), "Invalid collision layer");
		FR_ASSERT_MSG(is_collision_layer_valid(rhs), "Invalid collision layer");
		return _table[to_underlying(lhs)][to_underlying(rhs)];
	}

	void set(CollisionLayer lhs, CollisionLayer rhs, bool value) noexcept {
		FR_ASSERT_MSG(is_collision_layer_valid(lhs), "Invalid collision layer");
		FR_ASSERT_MSG(is_collision_layer_valid(rhs), "Invalid collision layer");
		_table[to_underlying(lhs)][to_underlying(rhs)] = value;
		_table[to_underlying(rhs)][to_underlying(lhs)] = value;
	}

	void add(CollisionLayer lhs, CollisionLayer rhs) noexcept {
		set(lhs, rhs, true);
	}

	void remove(CollisionLayer lhs, CollisionLayer rhs) noexcept {
		set(lhs, rhs, false);
	}

	/// @brief Add layer to the collision matrix so that it will collide with everything else
	void add_all_for(CollisionLayer layer) noexcept {
		FR_ASSERT_MSG(is_collision_layer_valid(layer), "Invalid collision layer");
		for (UnderlyingT i = 0; i <= static_cast<UnderlyingT>(max_collision_layer); ++i) {
			set(layer, static_cast<CollisionLayer>(i), true);
		}
	}

	/// @brief Remove layer from the collision matrix so that it will not collide with anything
	void remove_all_for(CollisionLayer layer) noexcept {
		FR_ASSERT_MSG(is_collision_layer_valid(layer), "Invalid collision layer");
		for (UnderlyingT i = 0; i <= static_cast<UnderlyingT>(max_collision_layer); ++i) {
			set(layer, static_cast<CollisionLayer>(i), false);
		}
	}

	void clear() {
		for (auto& row : _table) {
			row.fill(false);
		}
	}

private:
	std::array<std::array<bool, num_collision_layers>, num_collision_layers> _table = {};
};

/// @brief Description of the collision event of two entities
template<c_entity_id TEntityId>
class alignas(8) Collision {
public:
	struct Match {
		TEntityId first;
		TEntityId second;
	};

	constexpr
	Collision(
		TEntityId a, CollisionLayer layer_a,
		TEntityId b, CollisionLayer layer_b
	) noexcept {
		if (to_underlying(layer_a) <= to_underlying(layer_b)) {
			_entity1 = a;
			_layer1 = layer_a;
			_entity2 = b;
			_layer2 = layer_b;
		}
		else {
			_entity1 = b;
			_layer1 = layer_b;
			_entity2 = a;
			_layer2 = layer_a;
		}
	}

	auto match(
		CollisionLayer layer_a, CollisionLayer layer_b
	) const noexcept -> std::optional<Match> {
		// Sort categories in ascending order
		if (to_underlying(layer_a) <= to_underlying(layer_b)) {
			if (layer_a == _layer1 && layer_b == _layer2)
				return Match{_entity1, _entity2};
			else
				return std::nullopt;
		}
		else {
			if (layer_a == _layer2 && layer_b == _layer1)
				return Match{_entity2, _entity1};
			else
				return std::nullopt;
		}
	}

	FR_FORCE_INLINE constexpr
	auto first_entity() const noexcept -> TEntityId { return _entity1; }

	FR_FORCE_INLINE constexpr
	auto second_entity() const noexcept -> TEntityId { return _entity2; }

	FR_FORCE_INLINE constexpr
	auto first_layer() const noexcept -> CollisionLayer { return _layer1; }

	FR_FORCE_INLINE constexpr
	auto second_layer() const noexcept -> CollisionLayer { return _layer2; }

	friend
	auto operator==(Collision, Collision) -> bool = default;

private:
	TEntityId _entity1;
	TEntityId _entity2;
	CollisionLayer _layer1;
	CollisionLayer _layer2;
};

inline constexpr
auto bounding_radius(FAaRect box) noexcept -> float {
	return 0.5f * std::sqrt(sqr(box.width()) + sqr(box.height()));
}

inline constexpr
auto bounding_radius2(FAaRect box) noexcept -> float {
	return 0.25f * (sqr(box.width()) + sqr(box.height()));
}

auto calc_aabb(const glm::mat3& transform) noexcept -> FAaRect;

namespace experimental {

inline
auto colliders_intersect_aabb(const Collider& a, const Collider& b) noexcept -> bool {
	return shapes_intersect(a.unchecked_get<FAaRect>(), b.unchecked_get<FAaRect>());
}

auto colliders_intersect_visit(const Collider& a, const Collider& b) noexcept -> bool;
auto colliders_intersect_switch(const Collider& a, const Collider& b) noexcept -> bool;

} // namespace experimental

} // namespace fr
#endif // include guard
