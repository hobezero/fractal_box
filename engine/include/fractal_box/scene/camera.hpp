#ifndef FRACTAL_BOX_SCENE_CAMERA_HPP
#define FRACTAL_BOX_SCENE_CAMERA_HPP

#include "fractal_box/scene/transform.hpp"

namespace fr {

/// @brief Orthographic projection
struct OrthoProjection {
	/// @brief Width to height ratio
	float aspect_ratio = 1.f;
	/// @brief The smallest of two dimensions in world space
	float view_side = 1.f;

	auto make_matrix() const noexcept -> glm::mat3 {
		const auto size = aspect_ratio < 1.f
			? glm::vec2{view_side, view_side / aspect_ratio}
			: glm::vec2{view_side * aspect_ratio, view_side};
		return glm::scale(glm::mat3(1.f), 2.f / size);
	}
};

struct FreeCamera {
	Transform transform;
	OrthoProjection projection;

	auto make_view_proj_matrix() const noexcept -> glm::mat3 {
		return projection.make_matrix() * transform.make_view_matrix();
	}

	void set_viewport(glm::ivec2 viewpor_size) noexcept {
		projection.aspect_ratio = aspect_ratio(glm::vec2{viewpor_size});
	}
};

} // namespace fr
#endif // include guard
