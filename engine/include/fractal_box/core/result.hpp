#ifndef FRACTAL_BOX_CORE_RESULT_HPP
#define FRACTAL_BOX_CORE_RESULT_HPP

#include <memory>
#include <type_traits>
#include <variant>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

namespace detail {

template<class T>
using MapResultValue = typename MpLazyIf<std::is_void_v<T>>::template Type<std::monostate, T>;

} // namespace detail

template<class T, class... Errs>
requires (sizeof...(Errs) > 0zu) && c_mp_unique_pack<Errs...>
class Result {
	using Value = detail::MapResultValue<T>;
	using FirstErr = MpPackFirst<Errs...>;

public:
	using ValueType = T;
	using ErrorTypes = MpList<Errs...>;

	Result() = default;

	template<class... Args>
	requires (!c_void<T>) && std::constructible_from<T, Args...>
	FR_FORCE_INLINE constexpr
	Result(InPlaceInit, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>):
		_state{std::in_place_index<0zu>, std::forward<Args>(args)...}
	{ }

	FR_FORCE_INLINE constexpr
	Result(InPlaceInit) noexcept
	requires c_void<T>:
		_state{std::in_place_index<0zu>}
	{ }

	template<class... Args>
	requires (sizeof...(Errs) == 1zu) && std::constructible_from<FirstErr, Args...>
	FR_FORCE_INLINE constexpr
	Result(FromErrorInit, Args&&... args)
	noexcept(std::is_nothrow_constructible_v<FirstErr, Args...>):
		_state{std::in_place_index<1zu>, std::forward<Args>(args)...}
	{ }

	template<class E, class... Args>
	requires c_mp_pack_contains_once<E, Errs...>
	FR_FORCE_INLINE constexpr
	Result(FromErrorAsInit<E>, Args&&... args)
	noexcept(std::is_nothrow_constructible_v<E, Args...>):
		_state{std::in_place_type<E>, std::forward<Args>(args)...}
	{ }

	template<class U = std::remove_cv_t<T>>
	requires (!std::same_as<std::remove_cvref_t<U>, InPlaceInit>
		&& !std::same_as<std::remove_cvref_t<U>, FromErrorInit>
		&& !c_from_error_as_init<std::remove_cvref_t<U>>
		&& !std::same_as<std::remove_cvref_t<U>, Result>
		&& std::constructible_from<T, U>)
	explicit(!std::is_convertible_v<U, T>) FR_FORCE_INLINE constexpr
	Result(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>):
		_state{std::in_place_index<0zu>, std::forward<U>(value)}
	{ }

	template<class U = std::remove_cv_t<T>>
	requires (!std::same_as<std::remove_cvref_t<U>, Result>
		&& std::constructible_from<T, U>
		&& std::assignable_from<T&, U>)
	FR_FORCE_INLINE constexpr
	auto operator=(U&& value) noexcept(std::is_nothrow_assignable_v<T&, U>) -> Result& {
		_state.emplace<0zu>(std::forward<U>(value));
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return _state.index() == 0zu; }

	FR_FORCE_INLINE constexpr
	auto has_value() const noexcept { return _state.index() == 0zu; }

	template<c_mp_pack_contains_once<Errs...> E>
	FR_FORCE_INLINE constexpr
	auto has_error() const noexcept {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 1zu;
		return _state.index() == idx;
	}

	FR_FORCE_INLINE constexpr
	auto has_error() const noexcept
	requires (sizeof...(Errs) == 1zu) {
		return _state.index() == 1zu;
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto value(this Self&& self) noexcept -> FwdLike<T, Self> {
		FR_PANIC_CHECK(self._state.index() == 0zu);
		return std::get<0zu>(std::forward<Self>(self)._state);
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto operator*(this Self&& self) noexcept -> FwdLike<T, Self> {
		FR_PANIC_CHECK(self._state.index() == 0zu);
		return std::get<0zu>(std::forward<Self>(self)._state);
	}

	template<class Self>
	requires (!c_void<T>)
	constexpr FR_FLATTEN
	auto operator->(this Self&& self) noexcept -> CopyConst<T, Self>* {
		FR_PANIC_CHECK(self._state.index() == 0zu);
		return std::addressof(std::get<0zu>(std::forward<Self>(self)._state));
	}

	template<class Self>
	requires (sizeof...(Errs) == 1zu)
	constexpr FR_FLATTEN
	auto error(this Self&& self) noexcept -> FwdLike<MpPackFirst<Errs...>, Self> {
		FR_PANIC_CHECK(self._state.index() == 1zu);
		return std::get<1zu>(std::forward<Self>(self)._state);
	}

	template<c_mp_pack_contains_once<Errs...> E, class Self>
	constexpr FR_FLATTEN
	auto error(this Self&& self) noexcept -> FwdLike<E, Self> {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 1zu;
		FR_PANIC_CHECK(self._state.index() == idx);
		return std::get<idx>(std::forward<Self>(self)._state);
	}

	template<class U = std::remove_cv_t<T>>
	requires (!c_void<T>)
		&& std::copy_constructible<T>
		&& std::convertible_to<U, T>
	constexpr FR_FLATTEN
	auto value_or(U&& fallback) const&
	noexcept(c_nothrow_copy_constructible<T> && c_nothrow_explicitly_convertible_to<U, T>) -> T {
		if (_state.index() == 0zu)
			return std::get<0zu>(_state);
		return static_cast<T>(std::forward<U>(fallback));
	}

	template<class U = std::remove_cv_t<T>>
	requires (!c_void<T>)
		&& std::move_constructible<T>
		&& std::convertible_to<U, T>
	constexpr FR_FLATTEN
	auto value_or(U&& fallback) &&
	noexcept(c_nothrow_move_constructible<T> && c_nothrow_explicitly_convertible_to<U, T>) -> T {
		if (_state.index() == 0zu)
			return std::get<0zu>(std::move(_state));
		return static_cast<T>(std::forward<U>(fallback));
	}

	template<class E, class G = E>
	requires c_mp_pack_contains_once<E, Errs...>
		&& std::copy_constructible<E>
		&& std::convertible_to<G, E>
	constexpr FR_FLATTEN
	auto error_or(G&& fallback) const&
	noexcept(c_nothrow_copy_constructible<E> && c_nothrow_explicitly_convertible_to<G, E>) -> E {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 1zu;
		if (_state.index() == idx)
			return std::get<idx>(_state);
		return static_cast<E>(std::forward<G>(fallback));
	}

	template<class E, class G = E>
	requires c_mp_pack_contains_once<E, Errs...>
		&& std::move_constructible<E>
		&& std::convertible_to<G, E>
	constexpr FR_FLATTEN
	auto error_or(G&& fallback) &&
	noexcept(c_nothrow_move_constructible<E> && c_nothrow_explicitly_convertible_to<G, E>) -> E {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 1zu;
		if (_state.index() == idx)
			return std::get<idx>(std::move(_state));
		return static_cast<E>(std::forward<G>(fallback));
	}

	template<class G = FirstErr>
	requires (sizeof...(Errs) == 1zu)
		&& std::copy_constructible<FirstErr>
		&& std::convertible_to<G, FirstErr>
	constexpr FR_FLATTEN
	auto error_or(G&& fallback) const& noexcept(
		c_nothrow_copy_constructible<FirstErr> && c_nothrow_explicitly_convertible_to<G, FirstErr>
	) -> FirstErr {
		if (_state.index() == 1zu)
			return std::get<1zu>(_state);
		return static_cast<FirstErr>(std::forward<G>(fallback));
	}

	template<class G = FirstErr>
	requires (sizeof...(Errs) == 1zu)
		&& std::move_constructible<FirstErr>
		&& std::convertible_to<G, FirstErr>
	constexpr FR_FLATTEN
	auto error_or(G&& fallback) && noexcept(
		c_nothrow_move_constructible<FirstErr> && c_nothrow_explicitly_convertible_to<G, FirstErr>
	) -> FirstErr {
		if (_state.index() == 1zu)
			return std::get<1zu>(std::move(_state));
		return static_cast<FirstErr>(std::forward<G>(fallback));
	}

private:
	std::variant<Value, Errs...> _state;
};

} // namespace fr
#endif // include guard
