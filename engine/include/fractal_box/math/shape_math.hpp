#ifndef FRACTAL_BOX_MATH_SHAPE_MATH_HPP
#define FRACTAL_BOX_MATH_SHAPE_MATH_HPP

#include "fractal_box/core/math.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/math/shapes.hpp"

namespace fr {

// shape_perimeter(..)
// -------------------

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shape_perimeter(const glm::tvec2<T, Q>&) noexcept -> T {
	return T{0};
}

template<class T>
inline constexpr
auto shape_perimeter(const AaRect<T>& rect) noexcept -> T {
	return T{2} * (rect.width() + rect.height());
}

template<class T>
inline constexpr
auto shape_perimeter(const Circle<T>& circle) noexcept -> T {
	return T{2} * pi<T> * circle.radius();
}

// shape_contains(..)
// ------------------
// Check if each point of the second shape is also a point of the first shape (lies within it
// or lies on the boundary)

template<class T, glm::qualifier Q>
inline constexpr
auto shape_contains(const AaRect<T>& rect, const glm::tvec2<T, Q>& p) noexcept {
	return rect.min().x <= p.x && p.x <= rect.max().x
		&& rect.min().y <= p.y && p.y <= rect.max().y;
}

// shapes_intersect(..)
// --------------------
// Check whether two shapes have *any* common points

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shapes_intersect(const Point2d<T, Q>& point_a, const Point2d<T, Q>& point_b) noexcept -> bool {
	return point_a == point_b;
}

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shapes_intersect(const Point2d<T, Q>& point, const AaRect<T, Q>& rect) noexcept -> bool {
	return shape_contains(rect, point);
}

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shapes_intersect(const AaRect<T, Q>& rect, const Point2d<T, Q>& point) noexcept -> bool {
	return shape_contains(rect, point);
}

template<class T, glm::qualifier Q>
inline constexpr
auto shapes_intersect(const AaRect<T, Q>& rect_a, const AaRect<T, Q>& rect_b) noexcept -> bool {
	// TODO: Decide if we want inclusive or exclusive tests
	return rect_a.min().x <= rect_b.max().x
		&& rect_a.max().x >= rect_b.min().x;
}

template<class T, glm::qualifier Q>
inline constexpr
auto shapes_intersect(const Point2d<T, Q>& point, const Circle<T, Q>& circle) noexcept -> bool {
	const auto x = circle.center().x - point.x;
	const auto y = circle.center().y - point.y;
	const auto r = circle.radius();
	return sqr(x) + sqr(y) <= sqr(r);
}

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shapes_intersect(const Circle<T, Q>& circle, const Point2d<T, Q>& point) noexcept -> bool {
	return shapes_intersect(point, circle);
}

template<class T, glm::qualifier Q>
inline constexpr
auto shapes_intersect(const Circle<T, Q>& a, const Circle<T, Q>& b) noexcept -> bool {
	const auto x = a.center().x - b.center().x;
	const auto y = a.center().y - b.center().y;
	const auto r = a.radius() + b.radius();
	return sqr(x) + sqr(y) <= sqr(r);
}

template<class T, glm::qualifier Q>
inline constexpr
auto shapes_intersect(const Circle<T, Q>& circle, const AaRect<T, Q>& rect) noexcept -> bool {
	const auto closest = Point2d<T, Q>{
		clamp_between(circle.center().x, rect.min().x, rect.max().x),
		clamp_between(circle.center().y, rect.min().y, rect.max().y)
	};

	const auto d = circle.center() - closest;
	return sqr(d.x) + sqr(d.y) <= sqr(circle.radius());
}

template<class T, glm::qualifier Q>
FR_FORCE_INLINE constexpr
auto shapes_intersect(const AaRect<T, Q>& rect, const Circle<T, Q>& circle) noexcept -> bool {
	return shapes_intersect(circle, rect);
}

} // namespace fr
#endif // include guard
