#ifndef FRACTAL_BOX_CORE_ERROR_HANDLING_RESULT_HPP
#define FRACTAL_BOX_CORE_ERROR_HANDLING_RESULT_HPP

#include <memory>
#include <type_traits>
#include <variant>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

namespace detail {

template<class T>
concept c_special_result_type
	= std::same_as<T, InPlaceInit>
	|| std::same_as<T, FromErrorInit>
	|| c_from_error_as_init<T>;

} // namespace detail

/// @brief  A class similar to `std::expected` with the ability to hold one of many error types
/// @note FIXME: Unlike `std::expected`, provides no guarantee that it's never valueless by
/// exception
template<class T, class... Errs>
requires (sizeof...(Errs) > 0zu)
	&& c_mp_unique_pack<Errs...>
	&& (!detail::c_special_result_type<T>)
	&& (... && !detail::c_special_result_type<Errs>)
class Result {
	using ValueStorage = typename MpLazyIf<std::is_void_v<T>>::template Type<std::monostate, T>;
	using State = std::variant<ValueStorage, std::monostate, Errs...>;
	using FirstErr = MpPackFirst<Errs...>;

	template<class U, class... Gs>
	requires (sizeof...(Gs) > 0zu)
		&& c_mp_unique_pack<Gs...>
		&& (!detail::c_special_result_type<U>)
		&& (... && !detail::c_special_result_type<Gs>)
	friend class Result;

public:
	using ValueType = T;
	using ErrorTypes = MpList<Errs...>;

	template<class U>
	using Rebind = Result<U, Errs...>;

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
		_state{std::in_place_index<2zu>, std::forward<Args>(args)...}
	{ }

	template<class U, class... Gs>
	requires c_mp_subset_of<MpList<Gs...>, ErrorTypes>
	constexpr FR_FLATTEN
	Result(FromErrorInit, const Result<U, Gs...>& other):
		_state{std::visit([&other]<class G>(const G& err) -> State {
			static constexpr auto idx = mp_find<ErrorTypes, G> + 2zu;
			if constexpr (std::is_same_v<G, std::monostate>) {
				FR_ASSERT(false);
				return State{std::in_place_index<1zu>};
			}
			if constexpr (std::is_same_v<G, U>) {
				FR_ASSERT(other._state.index() != 0zu);
				return State{std::in_place_index<idx>, err};
			}
			else {
				return State{std::in_place_index<idx>, err};
			}
		}, other._state)}
	{ }

	template<class U, class... Gs>
	requires c_mp_subset_of<MpList<Gs...>, ErrorTypes>
	constexpr FR_FLATTEN
	Result(FromErrorInit, Result<U, Gs...>&& other):
		_state{std::visit([&other]<class G>(G&& err) -> State {
			static constexpr auto idx = mp_find<ErrorTypes, std::remove_reference_t<G>> + 2zu;
			if constexpr (std::is_same_v<std::remove_reference_t<G>, std::monostate>) {
				FR_ASSERT(false);
				return State{std::in_place_index<1zu>};
			}
			if constexpr (std::is_same_v<std::remove_reference_t<G>, U>) {
				FR_ASSERT(other._state.index() != 0zu);
				return State{std::in_place_index<idx>, std::move(err)};
			}
			else {
				return State{std::in_place_index<idx>, std::move(err)};
			}
		}, std::move(other)._state)}
	{ }

	template<class E, class... Args>
	requires c_mp_pack_contains_once<E, Errs...>
	FR_FORCE_INLINE constexpr
	Result(FromErrorAsInit<E>, Args&&... args)
	noexcept(std::is_nothrow_constructible_v<E, Args...>):
		_state{std::in_place_index<mp_find<ErrorTypes, E> + 2zu>, std::forward<Args>(args)...}
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
		_state.template emplace<0zu>(std::forward<U>(value));
		return *this;
	}

	template<class... Args>
	requires c_nothrow_constructible<T, Args...>
	FR_FORCE_INLINE constexpr
	auto emplace(Args&&... args) noexcept -> Result& {
		_state.template emplace<0zu>(std::forward<Args>(args)...);
		return *this;
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return _state.index() == 0zu; }

	FR_FORCE_INLINE constexpr
	auto has_value() const noexcept { return _state.index() == 0zu; }

	template<c_mp_pack_contains_once<Errs...> E>
	FR_FORCE_INLINE constexpr
	auto has_error() const noexcept {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 2zu;
		return _state.index() == idx;
	}

	FR_FORCE_INLINE constexpr
	auto has_error() const noexcept
	requires (sizeof...(Errs) == 1zu) {
		return _state.index() == 2zu;
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
		FR_PANIC_CHECK(self._state.index() == 2zu);
		return std::get<1zu>(std::forward<Self>(self)._state);
	}

	template<c_mp_pack_contains_once<Errs...> E, class Self>
	constexpr FR_FLATTEN
	auto error(this Self&& self) noexcept -> FwdLike<E, Self> {
		static constexpr auto idx = mp_find<ErrorTypes, E> + 2zu;
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
		static constexpr auto idx = mp_find<ErrorTypes, E> + 2zu;
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
		static constexpr auto idx = mp_find<ErrorTypes, E> + 2zu;
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
		if (_state.index() == 2zu)
			return std::get<2zu>(_state);
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
		if (_state.index() == 2zu)
			return std::get<2zu>(std::move(_state));
		return static_cast<FirstErr>(std::forward<G>(fallback));
	}

private:
	State _state;
};

} // namespace fr
#endif // include guard
