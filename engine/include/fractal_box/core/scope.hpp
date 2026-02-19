#ifndef FRACTAL_BOX_CORE_SCOPE_HPP
#define FRACTAL_BOX_CORE_SCOPE_HPP

#include <concepts>
#include <utility>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr {

template<class Action>
requires c_nothrow_move_constructible<Action> || c_nothrow_copy_constructible<Action>
class ScopeExit {
public:
	/// @todo FIXME: `std::experimental::scope_exit` is slightly different
	template<class U>
	requires std::constructible_from<Action, U> && (!std::same_as<U, ScopeExit>)
	explicit
	ScopeExit(U&& fn)
	noexcept(std::is_nothrow_constructible_v<Action, U>):
		_action{std::forward<U>(fn)},
		_is_active{true}
	{ }

	ScopeExit(const ScopeExit&) = delete;
	auto operator=(const ScopeExit&) -> ScopeExit& = delete;

	ScopeExit(ScopeExit&& other) noexcept:
		_action{std::forward<Action>(other._action)},
		_is_active{other._is_active}
	{
		other._is_active = false;
	}

	auto operator=(ScopeExit&&) -> ScopeExit& = delete;

	~ScopeExit() noexcept(noexcept(_action())) {
		if (_is_active)
			_action();
	}

	void release() noexcept { _is_active = false; }

	void execute() noexcept {
		if (_is_active)
			_action();
		_is_active = false;
	}

private:
	Action _action;
	bool _is_active;
};

template<class Action>
requires requires(Action f) { f(); }
ScopeExit(Action) -> ScopeExit<Action>;

namespace detail {

struct DeferToken {
	template<class F>
	friend constexpr
	auto operator|(DeferToken, F&& f) noexcept {
		return ScopeExit<std::remove_cvref_t<F>>{std::forward<F>(f)};
	}
};

} // namespace detail

#define FR_DEFER const auto FR_UNIQUE_NAME(kepler_defer_var) \
	= ::fr::detail::DeferToken{} |

} // namespace fr
#endif // include guard
