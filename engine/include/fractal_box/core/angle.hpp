#ifndef FRACTAL_BOX_CORE_ANGLE_HPP
#define FRACTAL_BOX_CORE_ANGLE_HPP

#include <concepts>

#include <glm/trigonometric.hpp>

namespace fr {

template<std::floating_point T>
class Rad;

template<std::floating_point T>
class Deg {
public:
	using ValueType = T;

	Deg() = default;

	explicit constexpr Deg(T value) noexcept: _value(value) { }

	constexpr Deg(Rad<T> rad) noexcept: _value(glm::degrees(rad.value())) { }

	constexpr auto value() const noexcept -> const T& { return _value; }

	explicit constexpr operator T() const noexcept { return _value; }

	constexpr auto operator-() const noexcept -> Deg { return Deg{-_value}; }
	constexpr auto operator+() const noexcept -> Deg { return *this; }

	friend constexpr
	auto operator+(Deg lhs, Deg rhs) noexcept -> Deg { return Deg{lhs._value + rhs._value}; }

	friend constexpr
	auto operator-(Deg lhs, Deg rhs) noexcept -> Deg { return Deg{lhs._value - rhs._value}; }

	friend constexpr
	auto operator*(Deg lhs, T rhs) noexcept -> Deg { return Deg{lhs._value * rhs}; }

	friend constexpr
	auto operator*(T lhs, Deg rhs) noexcept -> Deg { return Deg{lhs * rhs._value}; }

	friend constexpr
	auto operator/(Deg lhs, T rhs) noexcept -> Deg { return Deg{lhs._value / rhs}; }

	constexpr
	auto operator+=(Deg other) noexcept -> Deg& { _value += other._value; return *this; }

	constexpr
	auto operator-=(Deg other) noexcept -> Deg& { _value -= other._value; return *this; }

	constexpr
	auto operator*=(T other) noexcept -> Deg& { _value *= other; return *this; }

	constexpr
	auto operator/=(T other) noexcept -> Deg& { _value /= other; return *this; }

	auto operator<=>(this Deg lhs, Deg rhs) = default;

private:
	T _value {};
};

using FDeg = Deg<float>;
using DDeg = Deg<double>;

template<std::floating_point T>
class Rad {
public:
	using ValueType = T;

	Rad() noexcept = default;

	explicit constexpr Rad(T value) noexcept: _value(value) { }

	constexpr Rad(Deg<T> deg) noexcept: _value(glm::radians(deg.value())) { }

	constexpr auto value() const noexcept -> const T& { return _value; }

	explicit constexpr operator T() const noexcept { return _value; }

	constexpr auto operator-() const noexcept -> Rad { return Rad{-_value}; }
	constexpr auto operator+() const noexcept -> Rad { return *this; }

	friend constexpr
	auto operator+(Rad lhs, Rad rhs) noexcept -> Rad { return Rad{lhs._value + rhs._value}; }

	friend constexpr
	auto operator-(Rad lhs, Rad rhs) noexcept -> Rad { return Rad{lhs._value - rhs._value}; }

	friend constexpr
	auto operator*(Rad lhs, T rhs) noexcept -> Rad { return Rad{lhs._value * rhs}; }

	friend constexpr
	auto operator*(T lhs, Rad rhs) noexcept -> Rad { return Rad{lhs * rhs._value}; }

	friend constexpr
	auto operator/(Rad lhs, T rhs) noexcept -> Rad { return Rad{lhs._value / rhs}; }

	friend constexpr
	auto operator/(T lhs, Rad rhs) noexcept -> Rad { return Rad{lhs / rhs._value}; }

	constexpr
	auto operator+=(Rad other) noexcept -> Rad& { _value += other._value; return *this; }

	constexpr
	auto operator-=(Rad other) noexcept -> Rad& { _value -= other._value; return *this; }

	constexpr
	auto operator*=(T other) noexcept -> Rad& { _value *= other; return *this; }

	constexpr
	auto operator/=(T other) noexcept -> Rad& { _value /= other; return *this; }

	auto operator<=>(this Rad lhs, Rad rhs) = default;

private:
	T _value {};
};

using FRad = Rad<float>;
using DRad = Rad<double>;

} // namespace fr
#endif // include guard
