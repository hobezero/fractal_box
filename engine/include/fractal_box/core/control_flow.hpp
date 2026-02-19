#ifndef FRACTAL_BOX_CORE_CONTROL_FLOW_HPP
#define FRACTAL_BOX_CORE_CONTROL_FLOW_HPP

#include <concepts>
#include <utility>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

class ControlFlow {
	enum class Enum {
		Continue,
		Break,
		Return,
	};

public:
	// These statics violate naming conventions for constants, but it is because they mimic
	// enumerators. `ControlFlow` class should have been an enum anyway (in a better language).
	// NOTE: They are actually `constexpr`, see out-of-class definition below
	static const ControlFlow Continue;
	static const ControlFlow Break;
	static const ControlFlow Return;

	ControlFlow() = default;

	friend
	auto operator==(ControlFlow, ControlFlow) -> bool = default;

	FR_FORCE_INLINE constexpr
	auto is_continue() const noexcept -> bool { return _value == Enum::Continue; }

	FR_FORCE_INLINE constexpr
	auto is_break() const noexcept -> bool { return _value == Enum::Break; }

	FR_FORCE_INLINE constexpr
	auto is_return() const noexcept -> bool { return _value == Enum::Return; }

	FR_FORCE_INLINE constexpr
	auto is_break_or_return() const noexcept -> bool {
		return _value == Enum::Break || _value == Enum::Return;
	}

private:
	explicit FR_FORCE_INLINE constexpr
	ControlFlow(Enum value) noexcept: _value{value} { }

private:
	Enum _value = Enum::Continue;
};

// NOTE: `const` in declaration, `constexpr` in definition. The same trick is used by
// `std::strong_ordering` and its friends. Not sure why (and if) the Standard allows this
inline constexpr ControlFlow ControlFlow::Continue {ControlFlow::Enum::Continue};
inline constexpr ControlFlow ControlFlow::Break {ControlFlow::Enum::Break};
inline constexpr ControlFlow ControlFlow::Return {ControlFlow::Enum::Return};

template<class T>
concept c_void_or_control_flow = c_void<T> || std::same_as<T, ControlFlow>;

/// @note Does not utilize `std::invoke` because it would force us to include `<functional>`
template<class F, class... Args>
FR_FORCE_INLINE constexpr
auto cflow_invoke(F&& f, Args&&... args) noexcept(
	noexcept(std::forward<F>(f)(std::forward<Args>(args)...))
) -> decltype(auto) {
	using Ret = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
	if constexpr (std::is_void_v<Ret>) {
		std::forward<F>(f)(std::forward<Args>(args)...);
		return ControlFlow::Continue;
	}
	else {
		return std::forward<F>(f)(std::forward<Args>(args)...);
	}
}

} // namespace fr
#endif // include guard
