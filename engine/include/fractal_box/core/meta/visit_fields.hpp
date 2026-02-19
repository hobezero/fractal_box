#ifndef FRACTAL_BOX_CORE_META_VISIT_FIELDS_HPP
#define FRACTAL_BOX_CORE_META_VISIT_FIELDS_HPP

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/range_concepts.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

namespace detail {

template<class T, size_t BaseCount = 0zu, class... Args>
requires std::is_aggregate_v<T>
consteval auto aggregate_size() -> size_t {
	if constexpr (
		requires { T{Args{}...}; }
		&& !requires { T{Args{}..., ADH{}}; }
	) {
		return sizeof...(Args) - BaseCount;
	}
	else if constexpr (
		sizeof...(Args) == BaseCount
		&& requires { T{Args{}...}; }
		&& !requires { T{Args{}..., ADHEB<T>{}}; }
	) {
		return aggregate_size<T, BaseCount + 1zu, Args..., ADH>();
	}
	else {
		return aggregate_size<T, BaseCount, Args..., ADH>();
	}
}

template<class T>
inline consteval
auto num_record_fields_impl() -> size_t {
	using PT = std::remove_cvref_t<T>;
	if constexpr (std::is_array_v<PT>) {
		return std::extent_v<PT>;
	}
	else if constexpr (c_tuple_like<PT>) {
		return std::tuple_size_v<PT>;
	}
	else if constexpr (std::is_aggregate_v<PT>) {
		return detail::aggregate_size<PT>();
	}
	else if constexpr (c_constexpr_sized_range<PT>) {
		return constexpr_size<PT>;
	}
	else if constexpr (!std::is_class_v<PT>) {
		return 0;
	}
	else
		static_assert(false);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&&, ValueC<0zu>) -> decltype(auto) {
	return std::forward<F>(f);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<1zu>) -> decltype(auto) {
	auto&& [_0] = std::forward<T>(o);
	return std::forward<F>(f)(std::forward_like<T>(_0));
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<2zu>) -> decltype(auto) {
	auto&& [_0, _1] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<3zu>) -> decltype(auto) {
	auto&& [_0, _1, _2] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<4zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<5zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<6zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<7zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<8zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<9zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<10zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<11zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<12zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<13zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<14zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<15zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<16zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<17zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<18zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<19zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<20zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<21zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<22zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<23zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<24zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<25zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<26zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<27zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<28zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<29zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<30zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<31zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<32zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<33zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<34zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<35zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<36zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<37zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<38zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<39zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<40zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<41zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<42zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<43zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<44zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<45zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<46zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<47zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<48zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<49zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<50zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<51zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<52zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<53zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<54zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<55zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54]
		= std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<56zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<57zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<58zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<59zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<60zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58, _59] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58), std::forward_like<T>(_59)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<61zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58, _59, _60] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58), std::forward_like<T>(_59),
		std::forward_like<T>(_60)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<62zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58, _59, _60, _61] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58), std::forward_like<T>(_59),
		std::forward_like<T>(_60), std::forward_like<T>(_61)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<63zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58, _59, _60, _61, _62] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58), std::forward_like<T>(_59),
		std::forward_like<T>(_60), std::forward_like<T>(_61), std::forward_like<T>(_62)
	);
}

template<class F, class T>
FR_FORCE_INLINE constexpr
auto visit_record_impl(F&& f, T&& o, ValueC<64zu>) -> decltype(auto) {
	auto&& [_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,
		_19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,
		_37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54,
		_55, _56, _57, _58, _59, _60, _61, _62, _63] = std::forward<T>(o);
	return std::forward<F>(f)(
		std::forward_like<T>(_0), std::forward_like<T>(_1), std::forward_like<T>(_2),
		std::forward_like<T>(_3), std::forward_like<T>(_4), std::forward_like<T>(_5),
		std::forward_like<T>(_6), std::forward_like<T>(_7), std::forward_like<T>(_8),
		std::forward_like<T>(_9), std::forward_like<T>(_10), std::forward_like<T>(_11),
		std::forward_like<T>(_12), std::forward_like<T>(_13), std::forward_like<T>(_14),
		std::forward_like<T>(_15), std::forward_like<T>(_16), std::forward_like<T>(_17),
		std::forward_like<T>(_18), std::forward_like<T>(_19), std::forward_like<T>(_20),
		std::forward_like<T>(_21), std::forward_like<T>(_22), std::forward_like<T>(_23),
		std::forward_like<T>(_24), std::forward_like<T>(_25), std::forward_like<T>(_26),
		std::forward_like<T>(_27), std::forward_like<T>(_28), std::forward_like<T>(_29),
		std::forward_like<T>(_30), std::forward_like<T>(_31), std::forward_like<T>(_32),
		std::forward_like<T>(_33), std::forward_like<T>(_34), std::forward_like<T>(_35),
		std::forward_like<T>(_36), std::forward_like<T>(_37), std::forward_like<T>(_38),
		std::forward_like<T>(_39), std::forward_like<T>(_40), std::forward_like<T>(_41),
		std::forward_like<T>(_42), std::forward_like<T>(_43), std::forward_like<T>(_44),
		std::forward_like<T>(_45), std::forward_like<T>(_46), std::forward_like<T>(_47),
		std::forward_like<T>(_48), std::forward_like<T>(_49), std::forward_like<T>(_50),
		std::forward_like<T>(_51), std::forward_like<T>(_52), std::forward_like<T>(_53),
		std::forward_like<T>(_54), std::forward_like<T>(_55), std::forward_like<T>(_56),
		std::forward_like<T>(_57), std::forward_like<T>(_58), std::forward_like<T>(_59),
		std::forward_like<T>(_60), std::forward_like<T>(_61), std::forward_like<T>(_62),
		std::forward_like<T>(_63)
	);
}

} // namespace detail

template<class T>
inline constexpr auto num_record_fields = size_t{detail::num_record_fields_impl<T>()};

template<class F, class T>
requires c_record_like<std::remove_cvref_t<T>>
inline constexpr
auto visit_record_fields(T&& obj, F&& f) -> decltype(auto) {
	using PT = std::remove_cvref_t<T>;
	if constexpr (c_tuple_like<PT>) {
		return detail::visit_record_impl(std::forward<F>(f), std::forward<T>(obj),
			value_c<std::tuple_size_v<PT>>);
	}
	else {
		static_assert(num_record_fields<PT> <= 64, "Can't visit move than 64 fields");
		return detail::visit_record_impl(std::forward<F>(f), std::forward<T>(obj),
			value_c<num_record_fields<PT>>);
	}
}

} // namespace fr
#endif // include guard
