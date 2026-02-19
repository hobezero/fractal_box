#ifndef FRACTAL_BOX_CORE_MATH_HPP
#define FRACTAL_BOX_CORE_MATH_HPP

/// @file
/// @brief Math utility functions
/// @note We pass by value because parameters are generally small enough to fit into registers.
/// No slow pointer dereferencing in case a function doesn't get inlined

#include <cmath>
#include <concepts>
#include <limits>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "fractal_box/core/angle.hpp"
#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"

namespace fr {

/// @brief pi
template<std::floating_point T>
inline constexpr auto pi = std::numbers::pi_v<T>;

inline constexpr auto f_pi = pi<float>;
inline constexpr auto d_pi = pi<double>;

template<std::floating_point T>
inline constexpr auto pi_rad = Rad<T>{std::numbers::pi_v<T>};

inline constexpr auto f_pi_rad = pi_rad<float>;
inline constexpr auto d_pi_rad = pi_rad<double>;

template<std::floating_point T>
inline constexpr auto pi_deg = Deg<T>{pi_rad<T>};

inline constexpr auto f_pi_deg = pi_deg<float>;
inline constexpr auto d_pi_deg = pi_deg<double>;

/// @brief 2 * pi, also called tau
template<std::floating_point T>
inline constexpr auto tau = T{2} * pi<T>;

inline constexpr auto f_tau = tau<float>;
inline constexpr auto d_tau = tau<double>;

template<std::floating_point T>
inline constexpr auto tau_rad = Rad<T>{T{2} * pi<T>};

inline constexpr auto f_tau_rad = tau_rad<float>;
inline constexpr auto d_tau_rad = tau_rad<double>;

template<std::floating_point T>
inline constexpr auto tau_deg = Deg<T>{tau_rad<T>};

inline constexpr auto f_tau_deg = tau_deg<float>;
inline constexpr auto d_tau_deg = tau_deg<double>;

/// @brief iee754 positive infinity
template<std::floating_point T>
inline constexpr auto pos_inf = std::numeric_limits<T>::infinity();

inline constexpr auto f_pos_inf = pos_inf<float>;
inline constexpr auto d_pos_inf = pos_inf<double>;

/// @brief iee754 negative infinity
template<std::floating_point T>
inline constexpr auto neg_inf = -std::numeric_limits<T>::infinity();

inline constexpr auto f_neg_inf = neg_inf<float>;
inline constexpr auto d_neg_inf = neg_inf<double>;

/// @brief iee754 quiet NaN
template<std::floating_point T>
inline constexpr auto nan = std::numeric_limits<T>::quiet_NaN();

inline constexpr auto f_nan = nan<float>;
inline constexpr auto d_nan = nan<double>;

static_assert(std::numeric_limits<float>::has_quiet_NaN);
static_assert(std::numeric_limits<double>::has_quiet_NaN);

/// @brief Constrain a value to lie in the [min, max] interval
template<class T>
inline constexpr
auto clamp_between(T value, T min, T max) noexcept -> T {
	const T tmp = value < min ? min : value;
	return tmp < max ? tmp : max;
}

/// @brief Caluclate the square of a number
template<class T>
FR_FORCE_INLINE constexpr
auto sqr(T value) noexcept -> T {
	return value * value;
}

/// @brief Calculate whether two floating point numbers are equal within the desired precision
/// @param ulp Units in the last place. Larger ulp = larger margin for error => lower precision
/// @note Based on https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template<std::floating_point T>
inline constexpr
auto almost_equal(T x, T y, int ulp) noexcept -> bool {
	const T m = std::min(std::fabs(x), std::fabs(y));

	// Subnormal numbers have fixed exponent, which is `min_exponent - 1`.
	const int exp = m < std::numeric_limits<T>::min()
		? std::numeric_limits<T>::min_exponent - 1
		: std::ilogb(m);

	// We consider `x` and `y` equal if the difference between them is
	// within `n` ULPs.
	return std::fabs(x - y) <= static_cast<T>(ulp)
		* std::ldexp(std::numeric_limits<T>::epsilon(), exp);
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto almost_zero_len(glm::vec<2, T, Q> v, int ulp) noexcept -> bool {
	return almost_equal(v.x, T{0}, ulp)
		&& almost_equal(v.y, T{0}, ulp);
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto almost_unit_len(glm::vec<2, T, Q> v, int ulp) noexcept -> bool {
	return almost_equal(glm::dot(v, v), T{1}, ulp);
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto aspect_ratio(glm::vec<2, T, Q> vec) noexcept -> T {
	return vec.x / vec.y;
}

template<std::floating_point Result, c_arithmetic T>
FR_FORCE_INLINE constexpr
auto ratio_value(T numerator, T denominator) noexcept -> Result {
	return static_cast<Result>(numerator) / static_cast<Result>(denominator);
}

template<c_arithmetic T>
FR_FORCE_INLINE constexpr
auto f_ratio_value(T numerator, T denominator) noexcept -> float {
	return static_cast<float>(numerator) / static_cast<float>(denominator);
}
template<c_arithmetic T>
FR_FORCE_INLINE constexpr
auto d_ratio_value(T numerator, T denominator) noexcept -> double {
	return static_cast<double>(numerator) / static_cast<double>(denominator);
}

/// @brief Contruct a 2D unit vector from rotation angle
/// @param angle The angle in radians between +X axis and the requested vector
/// measured counter-clockwise (as in the trigonometric unit circle)
/// @return A 2D vector with length = 1 pointing in the requested direction
/// @note Can't use typedef aliases because templates
template<std::floating_point T>
inline constexpr
auto unit_dir(Rad<T> rotation) noexcept -> glm::vec<2, T> {
	const auto value = T{rotation};
	return {std::cos(value), std::sin(value)};
}

/// @brief Calculate rotation angle in [-pi; pi] interval of a 2D vector
/// @param vec Any vector. Is not required to be normalized
/// @return Angle in the [-pi; pi] inteval in radians
template<class T, glm::qualifier Q>
inline constexpr
auto vec_rotation(glm::vec<2, T, Q> vec) noexcept -> Rad<T> {
	// TODO: Replace with atan/acot to save on costly normalization
	const auto angle = std::acos(glm::normalize(vec).x);
	// equivalent to vec.y() < 0 : -angle : angle
	return Rad<T>{std::copysign(angle, vec.y)};
}

/// @brief Calculate rotation angle in [-pi; pi] interval of a normalized 2D vector
/// @param vec A normalized vector
/// @return Angle in the [-pi; pi] inteval in radians
template<class T, glm::qualifier Q>
inline constexpr
auto norm_vec_rotation(glm::vec<2, T, Q> vec) noexcept -> Rad<T> {
	FR_ASSERT_AUDIT(almost_equal(glm::dot(vec, vec), T{1}, 2));
	const auto angle = std::acos(vec.x);
	return Rad<T>{std::copysign(angle, vec.y)};
}

/// @brief Wrap any angle to [-pi; pi] interval using modular arithmetic
/// @param angle Any angle in radians
/// @return Angle in the [-pi; pi] interval in radians
template<class T>
[[nodiscard]] inline constexpr
auto wrap_angle_neg_pi_pi(Rad<T> angle) noexcept -> Rad<T> {
	const auto value = angle.value();
	if (value < -pi<T> || value > pi<T>) {
		auto normalized = std::fmod(value, tau<T>);
		if (normalized < 0.f) {
			normalized += tau<T>;
		}
		else if (normalized > pi<T>) {
			normalized -= tau<T>;
		}
		return Rad<T>{normalized};
	}
	return angle;
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto proportionally_resized_max(
	glm::vec<2, T, Q> src_size, T larger_target_dim
) noexcept -> glm::vec<2, T, Q> {
	const auto ratio = src_size.x / src_size.y;
	return ratio > T{1}
		? glm::vec<2, T, Q>{larger_target_dim, larger_target_dim / ratio}
		: glm::vec<2, T, Q>{larger_target_dim * ratio, larger_target_dim};
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto proportionally_resized_min(
	glm::vec<2, T, Q> src_size, T smaller_target_dim
) noexcept {
	const auto ratio = src_size.x / src_size.y;
	return ratio > T{1}
		? glm::vec<2, T, Q>{smaller_target_dim * ratio, smaller_target_dim}
		: glm::vec<2, T, Q>{smaller_target_dim, smaller_target_dim / ratio};
}

template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto rotated_by(glm::vec<2, T, Q> vec, Rad<T> angle) noexcept -> glm::vec<2, T, Q> {
	const auto sine = std::sin(angle.value());
	const auto cosine = std::cos(angle.value());
	return {
		vec.x * cosine - vec.y * sine,
		vec.x * sine + vec.y * cosine
	};
}

/// @brief Calculates the midpoint of two floating-point values a and b
/// @return Half the sum of a and b
/// @warning Doesn't work well with huge numbers (over hallf of infinity) and
/// possibly denormalized numbers. For that, use C++20 std::midpoint.
/// Designed for speed in simple cases
template<std::floating_point T>
inline constexpr
auto midpoint(T a, T b) noexcept -> T {
	// This formula would upset Marshall Clow but we are not libc++, are we?
	return (a + b) / T{2};
}

/// @brief Calculates the midpoint of two floating-point 2D vectors
/// @return Half the sum of a and b
template<std::floating_point T, glm::qualifier Q>
inline constexpr
auto midpoint(glm::vec<2, T, Q> a, glm::vec<2, T, Q> b) noexcept -> glm::vec<2, T, Q> {
	return (a + b) / T{2};
}

} // namespace fr
#endif // include guard
