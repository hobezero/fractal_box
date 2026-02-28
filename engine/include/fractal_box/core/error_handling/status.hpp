#ifndef FRACTAL_BOX_CORE_ERROR_HANDLING_STATUS_HPP
#define FRACTAL_BOX_CORE_ERROR_HANDLING_STATUS_HPP

#include <optional>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T>
class Status;

template<class T>
inline constexpr auto is_status = false;

template<class T>
inline constexpr auto is_status<Status<T>> = true;

template<class T>
concept c_status = is_status<T>;

template<class T = void>
class Status {
	using State = typename MpLazyIf<std::is_void_v<T>>::template Type<bool, std::optional<T>>;

public:
	using ValueType = T;

	FR_FORCE_INLINE constexpr
	Status() noexcept(std::is_nothrow_default_constructible_v<T>)
	requires (!c_void<T>) && c_default_constructible<T>:
		_state{std::in_place}
	{ }

	FR_FORCE_INLINE constexpr
	Status() noexcept
	requires c_void<T>:
		_state{true}
	{ }

	FR_FORCE_INLINE constexpr
	Status(FromErrorInit) noexcept
	requires (!c_void<T>): _state{std::nullopt} { }

	FR_FORCE_INLINE constexpr
	Status(FromErrorInit) noexcept
	requires c_void<T>: _state{false} { }

	template<class... Args>
	requires (!c_void<T>) && std::constructible_from<T, Args...>
	FR_FORCE_INLINE constexpr
	Status(InPlaceInit, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>):
		_state{std::in_place, std::forward<Args>(args)...}
	{ }

	FR_FORCE_INLINE constexpr
	Status(InPlaceInit) noexcept
	requires c_void<T>:
		_state{true}
	{ }

	template<class U = std::remove_cv_t<T>>
	requires (!c_void<T>)
		&& std::constructible_from<T, U>
		&& (!std::same_as<std::remove_cvref_t<U>, InPlaceInit>)
		&& (!std::same_as<std::remove_cvref_t<U>, FromErrorInit>)
		&& (!std::same_as<std::remove_cvref_t<U>, Status>)
		&& (!std::same_as<T, bool> || !c_status<std::remove_cvref_t<U>>)
	explicit(!std::is_convertible_v<U, T>) FR_FORCE_INLINE constexpr
	Status(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>):
		_state{std::forward<U>(value)}
	{ }

	FR_FORCE_INLINE constexpr
	auto operator=(FromErrorInit) noexcept -> Status& {
		_state = std::nullopt;
		return *this;
	}

	template<class U = std::remove_cv_t<T>>
	requires (!c_void<T>)
		&& (!std::same_as<std::remove_cvref_t<U>, Status>)
		&& std::constructible_from<T, U>
		&& std::assignable_from<T&, U>
		// && (!c_scalar<T> || !std::same_as<std::decay_t<U>, T>)
	FR_FORCE_INLINE constexpr
	auto operator=(U&& value) noexcept(
		std::is_nothrow_assignable_v<T&, U> && std::is_nothrow_constructible_v<T, U>
	) -> Status& {
		_state = std::forward<U>(value);
		return *this;
	}

	friend FR_FORCE_INLINE constexpr
	auto swap(Status& lhs, Status& rhs) noexcept(std::is_nothrow_swappable_v<State>) {
		using std::swap;
		swap(lhs._state, rhs._state);
	}

	template<class... Args>
	requires (!c_void<T>) && std::constructible_from<T, Args...>
	FR_FORCE_INLINE constexpr
	auto emplace(Args&&... args) noexcept(
		std::is_nothrow_constructible_v<T, Args...>
	) -> decltype(auto) {
		return _state.emplace(std::forward<Args>(args)...);
	}

	FR_FORCE_INLINE constexpr
	void reset() noexcept {
		if constexpr (std::is_void_v<T>) {
			_state = false;
		}
		else {
			_state.reset();
		}
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept {
		if constexpr (std::is_void_v<T>) {
			return _state;
		}
		else {
			return _state.has_value();
		}
	}

	FR_FORCE_INLINE constexpr
	auto has_value() const noexcept -> bool {
		if constexpr (std::is_void_v<T>) {
			return _state;
		}
		else {
			return _state.has_value();
		}
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto value(this Self&& self) noexcept -> FwdLike<T, Self> {
		FR_PANIC_CHECK(self._state.has_value());
		return *std::forward<Self>(self)._state;
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto operator*(this Self&& self) noexcept -> FwdLike<T, Self> {
		FR_PANIC_CHECK(self._state.has_value());
		return *std::forward<Self>(self)._state;
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto operator->(this Self&& self) noexcept -> CopyConst<T, Self>* {
		FR_PANIC_CHECK(self._state.has_value());
		return std::forward<Self>(self)._state.operator->();
	}

private:
	State _state;
};

} // namespace fr
#endif // include guard
