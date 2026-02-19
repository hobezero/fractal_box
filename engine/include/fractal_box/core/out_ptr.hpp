#ifndef FRACTAL_BOX_CORE_OUT_PTR_HPP
#define FRACTAL_BOX_CORE_OUT_PTR_HPP

#include <memory>

#include "fractal_box/core/platform.hpp"

namespace fr {

template<class Traits>
class OutPtr {
public:
	using Parent = typename Traits::Parent;
	using ValueType = typename Traits::ValueType;

	explicit constexpr
	OutPtr(Parent& parent) noexcept(noexcept(Traits::get_impl(parent))):
		_parent{std::addressof(parent)},
		_value{Traits::get_impl(parent)}
	{ }

	OutPtr(const OutPtr&) = delete;
	auto operator=(const OutPtr&) -> OutPtr& = delete;

	OutPtr(OutPtr&&) = delete;
	auto operator=(OutPtr&&) -> OutPtr& = delete;

	~OutPtr() noexcept(noexcept(Traits::set_impl(*_parent, _value))) {
		Traits::set_impl(*_parent, _value);
	}

	explicit(false) FR_FORCE_INLINE constexpr
	operator ValueType*() noexcept { return std::addressof(_value); }

private:
	Parent* _parent;
	ValueType _value;
};

template<class T>
concept c_with_enabled_funcs = requires(T x, bool value) {
	{ x.is_enabled() } -> std::convertible_to<bool>;
	x.set_enabled(value);
};

template<c_with_enabled_funcs P>
struct OutPtrForEnabledTraits {
	using Parent = P;
	using ValueType = bool;

	static FR_FORCE_INLINE
	auto get_impl(const Parent& parent) noexcept(noexcept(parent.is_enabled())) -> bool {
		return parent.is_enabled();
	}

	static FR_FORCE_INLINE
	void set_impl(Parent& parent, bool value) noexcept(noexcept(parent.set_enabled(value))) {
		parent.set_enabled(value);
	}
};

template<c_with_enabled_funcs Parent>
using OutPtrForEnabled = OutPtr<OutPtrForEnabledTraits<Parent>>;

template<c_with_enabled_funcs Parent>
FR_FORCE_INLINE constexpr
auto out_ptr_for_enabled(Parent& parent) -> OutPtrForEnabled<Parent> {
	return OutPtrForEnabled<Parent>{parent};
}

} // namespace fr
#endif
