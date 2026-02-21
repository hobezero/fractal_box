#ifndef FRACTAL_BOX_PHYSICS_NUMERICAL_HPP
#define FRACTAL_BOX_PHYSICS_NUMERICAL_HPP

#include <glm/vec2.hpp>

namespace fr {

template<class T, glm::qualifier Q>
struct EulerResult {
	glm::vec<2, T, Q> velocity {};
	glm::vec<2, T, Q> position {};
};

template<class T, glm::qualifier Q>
inline constexpr
auto euler_step(
	glm::vec<2, T, Q> velocity, glm::vec<2, T, Q> position,
	glm::vec<2, T, Q> acceleration, T dt
) noexcept -> EulerResult<T, Q> {
	EulerResult<T, Q> next;
	next.velocity = velocity + acceleration * dt;
	next.position = position + next.velocity * dt;
	return next;
}

} // namespace fr
#endif // include guard
