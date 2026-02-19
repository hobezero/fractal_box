#ifndef FRACTAL_BOX_CORE_IN_PLACE_ARGS_HPP
#define FRACTAL_BOX_CORE_IN_PLACE_ARGS_HPP

#include <tuple>
#include <utility>
#include <type_traits>

#include "fractal_box/core/meta//meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T, class... TArgs>
struct InPlaceArgs {
	using Type = T;
	using Args = MpList<TArgs...>;

	explicit FR_FORCE_INLINE constexpr FR_FLATTEN
	InPlaceArgs(TArgs... values) noexcept:
		args{std::forward<TArgs>(values)...}
	{ }

public:
	std::tuple<TArgs...> args;
};

template<class T, class... Args>
FR_FORCE_INLINE constexpr
auto in_place_args(Args&&... args) noexcept -> InPlaceArgs<T, Args&&...> {
	return InPlaceArgs<T, Args&&...>{std::forward<Args>(args)...};
}

template<class T>
inline constexpr auto is_in_place_args = false;

template<class T, class... Args>
inline constexpr auto is_in_place_args<InPlaceArgs<T, Args...>> = true;

template<class T>
using IsInPlaceArgs = BoolC<is_in_place_args<T>>;

template<class T>
concept c_in_place_args = is_in_place_args<T>;

namespace detail {

template<class T>
struct UnwrapInPlaceArgsImpl {
	using Type = T;
};

template<class T, class... Args>
struct UnwrapInPlaceArgsImpl<InPlaceArgs<T, Args...>> {
	using Type = T;
};

} // namespace detail

template<class T>
using UnwrapInPlaceArgs = typename detail::UnwrapInPlaceArgsImpl<std::remove_cvref_t<T>>::Type;

} // namespace fr
#endif // include guard
