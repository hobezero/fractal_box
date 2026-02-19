#ifndef FRACTAL_BOX_CORE_SHAPES_HPP
#define FRACTAL_BOX_CORE_SHAPES_HPP

#include <glm/vec2.hpp>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/functional.hpp"

namespace fr {

struct FromCenterInit {
	explicit FromCenterInit() = default;
};

inline constexpr auto from_center = FromCenterInit{};

template<class T, glm::qualifier Q = glm::defaultp>
using Point2d = glm::tvec2<T, Q>;

using FPoint2d = Point2d<float>;
using DPoint2d = Point2d<double>;

/// @brief Axis-aligned rectangle
template<std::floating_point T, glm::qualifier Q = glm::defaultp>
class AaRect {
public:
	using ValueType = T;
	using PointType = Point2d<T, Q>;
	using VecType = glm::tvec2<T, Q>;

	AaRect() = default;

	constexpr
	AaRect(PassAbi<PointType> min, PassAbi<PointType> max) noexcept:
		_min{min},
		_max{max}
	{
		FR_ASSERT_AUDIT(min.x <= max.x && min.y <= min.y);
	}

	constexpr
	AaRect(
		FromCenterInit, PassAbi<PointType> center, PassAbi<VecType> half_size
	) noexcept:
		_min{center - half_size},
		_max{center + half_size}
	{ }

	constexpr
	auto padded(PassAbi<VecType> pad) const noexcept -> AaRect {
		return {_min - pad, _max + pad};
	}

	constexpr
	auto center() const noexcept -> PointType {
		return _min + T{0.5} * size();
	}

	constexpr
	auto size() const noexcept -> VecType { return _max - _min; }

	constexpr auto x_length() const noexcept -> ValueType { return _max.x - _min.x; }
	constexpr auto width() const noexcept -> ValueType { return _max.x - _min.x; }
	constexpr auto y_length() const noexcept -> ValueType { return _max.y - _min.y; }
	constexpr auto height() const noexcept -> ValueType { return _max.y - _min.y; }

	constexpr auto min() const noexcept -> PassAbi<PointType> { return _min; }
	constexpr auto max() const noexcept -> PassAbi<PointType> { return _max; }

private:
	PointType _min {};
	PointType _max {};
};

using FAaRect = AaRect<float, glm::defaultp>;
using DAaRect = AaRect<double, glm::defaultp>;

template<std::floating_point T, glm::qualifier Q = glm::defaultp>
class Circle {
public:
	using ValueType = T;
	using PointType = Point2d<T, Q>;

	Circle() = default;

	constexpr
	Circle(PassAbi<PointType> center, T radius) noexcept:
		_center{center},
		_radius{radius}
	{
		FR_ASSERT_AUDIT(_radius >= T{0});
	}

	constexpr
	auto calc_aabb() const noexcept -> AaRect<T, Q> {
		return {from_center, _center, glm::tvec2<T, Q>{_radius, _radius}};
	}

	constexpr auto center() const noexcept -> PassAbi<PointType> { return _center; }
	constexpr auto set_center(PassAbi<PointType> center) noexcept { _center = center; }
	constexpr auto radius() const noexcept -> ValueType { return _radius; }
	constexpr auto set_radius(ValueType radius) noexcept { _radius = radius; }

private:
	PointType _center {};
	ValueType _radius {};
};

using FCircle = Circle<float, glm::defaultp>;
using DCircle = Circle<double, glm::defaultp>;

} // namespace fr
#endif // include guard
