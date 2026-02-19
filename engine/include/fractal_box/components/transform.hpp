#ifndef FRACTAL_BOX_COMPONENTS_TRANSFORM_HPP
#define FRACTAL_BOX_COMPONENTS_TRANSFORM_HPP

#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/mat3x3.hpp>

#include "fractal_box/core/math.hpp"
#include "fractal_box/runtime/world_types.hpp"

namespace fr {

struct TranslationTransform {
	auto make_model_matrix() const noexcept -> glm::mat3 {
		return glm::translate(glm::mat3(1.f), this->position);
	}

public:
	glm::vec2 position {0.f, 0.f};
};

/// @brief A Transform without rotation
struct ParallelTransform {
	auto make_model_matrix() const noexcept -> glm::mat3 {
		return glm::translate(glm::mat3(1.f), this->position)
			* glm::scale(glm::mat3(1.f), this->scale);
	}

public:
	/// @brief Translation (position) of transform
	glm::vec2 position {0.f, 0.f};
	/// @brief Scale (size) of transform
	glm::vec2 scale {1.f, 1.f};

};

struct TransRotationTransform {
	auto make_model_matrix() const noexcept -> glm::mat3 {
		return glm::translate(glm::mat3(1.f), this->position)
			* glm::rotate(glm::mat3(1.f), this->rotation.value());
	}

public:
	/// @brief Translation (position) of transform. +Y axis is up. +X axis is right
	glm::vec2 position {0.f, 0.f};
	/// @brief The angle between worlds (or parents) +X axis and transforms +X axis
	/// measured counter-clockwise
	FRad rotation {0.f};
};

/// @brief A subset of affine transformations
///
/// Applied in the following order: scaling, rotating, translation
struct Transform {
	Transform() noexcept = default;

	constexpr
	Transform(
		glm::vec2 t_position,
		FRad t_rotation = FRad{0.f},
		glm::vec2 t_scale = {1.f, 1.f}
	) noexcept:
		position(t_position),
		rotation(t_rotation),
		scale(t_scale)
	{ }

	constexpr
	Transform(
		TransRotationTransform trans_rotation,
		glm::vec2 t_scale = {1.f, 1.f}
	) noexcept:
		position(trans_rotation.position),
		rotation(trans_rotation.rotation),
		scale(t_scale)
	{ }

	constexpr
	Transform(
		ParallelTransform parallel,
		FRad t_rotation = FRad{0.f}
	) noexcept:
		position(parallel.position),
		rotation(t_rotation),
		scale(parallel.scale)
	{ }

	auto make_model_matrix() const noexcept -> glm::mat3 {
		return glm::translate(glm::mat3(1.f), this->position)
			* glm::rotate(glm::mat3(1.f), this->rotation.value())
			* glm::scale(glm::mat3(1.f), this->scale);
	}

	auto make_trans_rotation_matrix() const noexcept -> glm::mat3 {
		return glm::translate(glm::mat3(1.f), this->position)
			* glm::rotate(glm::mat3(1.f), this->rotation.value());
	}

	auto make_view_matrix() const noexcept -> glm::mat3 {
		return glm::rotate(glm::mat3(1.f), this->rotation.value())
			* glm::translate(glm::mat3(1.f), this->position);
	}

public:
	/// @brief Translation (position) of transform. +Y axis is up. +X axis is right
	glm::vec2 position {0.f, 0.f};
	/// @brief The angle between worlds (or parents) +X axis and transforms +X axis
	/// measured counter-clockwise
	FRad rotation {0.f};
	/// @brief Scale (size) of transform
	glm::vec2 scale {1.f, 1.f};
};

inline
auto transformed_by(
	TransRotationTransform local, const Transform& base
) noexcept -> TransRotationTransform {
	return {
		base.position + rotated_by(local.position, base.rotation),
		base.rotation + local.rotation
	};
}

template<c_entity_id TEntityId>
struct ParentRelationship {
	TEntityId parent;
	TransRotationTransform local_transform {};
};

} // namespace fr
#endif // include guard
