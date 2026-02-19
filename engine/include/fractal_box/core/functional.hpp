#ifndef FRACTAL_BOX_CORE_FUNCTIONAL_HPP
#define FRACTAL_BOX_CORE_FUNCTIONAL_HPP

#include <type_traits>
#include <concepts>
#include <utility>

#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

struct TruePredicate {
	static constexpr
	auto operator()(auto&&...) noexcept -> bool {
		return true;
	}
};

struct FalsePredicate {
	static constexpr
	auto operator()(auto&&...) noexcept -> bool {
		return false;
	}
};

inline constexpr auto true_pred = TruePredicate{};
inline constexpr auto false_pred = FalsePredicate{};

struct NoOp {
	static FR_FORCE_INLINE constexpr
	void operator()(auto&&...) noexcept {
		// Do literally nothing
	}
};

struct Identity {
	using is_transparent = void;

	template<class T>
	static FR_FORCE_INLINE constexpr
	auto operator()(T&& value) noexcept -> T&& {
		return std::forward<T>(value);
	}
};

/// @brief Equivalent std::equal_to<T> but doesn't require <functional> header and
/// is conditionally noexcept
template<class T = void>
struct EqualTo;

template<std::equality_comparable T>
struct EqualTo<T> {
	static FR_FORCE_INLINE constexpr
	auto operator()(const T& lhs, const T& rhs)
	noexcept(noexcept(lhs == rhs)) -> bool {
		return lhs == rhs;
	}
};

/// @brief Equivalent std::equal_to<void> but doesn't require <functional> header
template<>
struct EqualTo<void> {
	using is_transparent = void;

	template<class T, class U>
	requires std::equality_comparable_with<T, U>
	FR_FORCE_INLINE constexpr
	auto operator()(T&& lhs, U&& rhs) const
	noexcept(noexcept(std::forward<T>(lhs) == std::forward<U>(rhs))) -> bool {
		return std::forward<T>(lhs) == std::forward<U>(rhs);
	}
};

/// @brief Wraps a function pointer into a callable object by storing the pointer in its type.
/// "Lifts" a runtime (from the type system POV) pointer into a compile-time type
/// @todo TODO: Use `std::invoke` or equivalent. Support pointers to member functions
template<auto Function>
requires (std::is_function_v<std::remove_cvref_t<std::remove_pointer_t<decltype(Function)>>>)
struct FuncLifter {
	template<class... Args>
	static FR_FORCE_INLINE constexpr
	auto operator()(Args&&... args)
	noexcept(noexcept(Function(std::forward<Args>(args)...))) -> decltype(auto)
	requires requires { Function(std::forward<Args>(args)...); } {
		return Function(std::forward<Args>(args)...);
	}
};

template<class F>
FR_FORCE_INLINE constexpr
auto lift_bool(bool value, F&& f)
noexcept(noexcept(f(true_c)) && noexcept(f(false_c))) -> decltype(auto)
requires requires { f(true_c); f(false_c); } {
	if (value)
		return f(true_c);
	else
		return f(false_c);
}

/// @brief Combines multiple function objects (usually lambdas) into one
///
/// Gives an illusion that `std::visit` supports pattern matching
template<class... Fs>
struct Overload: Fs...{
	template<class...Ts>
	explicit(false) constexpr
	Overload(Ts&&... ts) noexcept((noexcept(Fs{std::forward<Ts>(ts)}) && ...)):
		Fs{std::forward<Ts>(ts)}...
	{ }

	using Fs::operator()...;
};

template<class... Ts>
Overload(Ts&&...) -> Overload<std::remove_reference_t<Ts>...>;

template<class T>
concept c_fast_by_value
	= c_object<T> && !c_ref<T>
	// SystemV requirements (https://www.uclibc.org/docs/psABI-x86_64.pdf#subsection.3.2.3)
	&& sizeof(T) <= 2zu * sizeof(int*)
	&& std::is_trivially_copy_constructible_v<T>
	&& std::is_trivially_destructible_v<T>
	&& std::is_trivially_copyable_v<T> // ???
	// Additional Itanium C++ requirements (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#non-trivial)
	&& std::is_trivially_move_constructible_v<T>
;

/// @brief The most efficient platform-specific way to pass `T` as a parameter to a function
/// (or return from one). When possible, objects should be passed in CPU registers
/// @note Currently, assumes that the platform complies with SystemV AMD64/Itanium C++ calling
/// conventions
/// @warning Doesn't work with deduced parameter types
/// @todo TODO: Support Windows/ARM/WASM/other platforms
template<c_object T>
using PassAbi = MpLazyIf<c_fast_by_value<T>>::template Type<std::decay_t<T>, const T&>;

namespace detail {

template<class F>
struct MemberTypeImpl;

template<class C, class M>
struct MemberTypeImpl<M C::*> {
	using ClassType = C;
	using Type = M;
};

} // namespace detail

template<class T>
using MemberType = typename detail::MemberTypeImpl<T>::Type;

template<class T>
using MemberClassType = typename detail::MemberTypeImpl<T>::ClassType;

template<auto MemberPtr>
requires std::is_member_object_pointer_v<decltype(MemberPtr)>
	&& std::equality_comparable<MemberType<decltype(MemberPtr)>>
struct MemberEqualTo {
	using is_transparent = void;

	using ClassType = MemberClassType<decltype(MemberPtr)>;
	using ValueType = MemberType<decltype(MemberPtr)>;

	static FR_FORCE_INLINE constexpr
	auto operator()(PassAbi<ClassType> lhs, PassAbi<ClassType> rhs)
	noexcept(noexcept(lhs.*MemberPtr == rhs.*MemberPtr)) -> bool {
		return lhs.*MemberPtr == rhs.*MemberPtr;
	}

	template<std::equality_comparable_with<ValueType> U>
	static FR_FORCE_INLINE constexpr
	auto operator()(PassAbi<ClassType> lhs, const U& rhs)
	noexcept(noexcept(lhs.*MemberPtr == rhs)) -> bool {
		return lhs.*MemberPtr == rhs;
	}

	template<std::equality_comparable_with<ValueType> U>
	static FR_FORCE_INLINE constexpr
	auto operator()(const U& lhs, PassAbi<ClassType> rhs)
	noexcept(noexcept(lhs == rhs.*MemberPtr)) -> bool {
		return lhs == rhs.*MemberPtr;
	}
};

enum class FuncQualifier: unsigned {
	Const = 1u << 0,
	Volatile = 1u << 1,
	LValueRef = 1u << 2,
	RValueRef = 1u << 3,
	Noexcept = 1u << 4, // Technically, not a qualifier, but a specifier
};

using FuncQualifiers = Flags<FuncQualifier>;

enum class CallableKind {
	/// @brief Non-member or static member funciton
	FreeFunction,
	/// @brief Non-static member function
	MemberFunction,
	/// @brief User-defined type that has an overloaded `operator()`
	Class,
};

namespace detail {

template<class Ret, class... Args>
struct FuncTraitsBase {
	using Stripped = Ret (Args...);
	static constexpr auto stripped = mp_type<Ret (Args...)>;

	using Result = Ret;
	static constexpr auto result = mp_type<Ret>;

	using Arguments = MpList<Args...>;
	static constexpr auto arguments = mp_list<Args...>;
};

template<class F>
struct FuncTraitsImpl;

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...)>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{};
};

// CV-qualifiers
// ^^^^^^^^^^^^^

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{FuncQualifier::Const};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{FuncQualifier::Volatile};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile}};
};

// Reference qaulifiers
// ^^^^^^^^^^^^^^^^^^^^

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) &>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{FuncQualifier::LValueRef};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const &>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::LValueRef}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile &>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Volatile,
		FuncQualifier::LValueRef}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile &>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::LValueRef}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) &&>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{FuncQualifier::RValueRef};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const &&>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::RValueRef}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile &&>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::RValueRef}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile &&>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::RValueRef}};
};

// noexcept
// ^^^^^^^^

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{FuncQualifier::Noexcept};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Volatile,
		FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) & noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::LValueRef,
		FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const & noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::LValueRef, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile & noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Volatile,
		FuncQualifier::LValueRef, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile & noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::LValueRef, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) && noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::RValueRef,
		FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const && noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::RValueRef, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) volatile && noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Volatile,
		FuncQualifier::RValueRef, FuncQualifier::Noexcept}};
};

template<class Ret, class... Args>
struct FuncTraitsImpl<Ret (Args...) const volatile && noexcept>: FuncTraitsBase<Ret, Args...> {
	static constexpr auto qualifiers = FuncQualifiers{{FuncQualifier::Const,
		FuncQualifier::Volatile, FuncQualifier::RValueRef, FuncQualifier::Noexcept}};
};

// TODO: So much repetition... Is there a better way of doing this (modulo macros)?

} // namespace detail

/// @brief Query information about a callable type at compile time
/// @note C-style variadic functions are NOT supported
template<class F>
struct FuncTraits;

template<class F>
requires std::is_function_v<std::remove_cvref_t<std::remove_pointer_t<F>>>
struct FuncTraits<F>: detail::FuncTraitsImpl<std::remove_cvref_t<std::remove_pointer_t<F>>> {
	static constexpr auto kind = CallableKind::FreeFunction;
};

template<class C, c_function M>
struct FuncTraits<M C::*>: detail::FuncTraitsImpl<M> {
	static constexpr auto kind = CallableKind::MemberFunction;
	using ClassType = C;
	static constexpr auto class_type = mp_type<C>;
};

template<class C>
requires requires { &std::remove_cvref_t<C>::operator(); }
struct FuncTraits<C>: FuncTraits<decltype(&std::remove_cvref_t<C>::operator())> {
	static constexpr auto kind = CallableKind::Class;
};

template<class T>
concept c_callable = is_complete<FuncTraits<T>>;

template<class T>
concept c_static_callable
	= c_callable<T> &&
	(FuncTraits<T>::kind == CallableKind::FreeFunction
	|| (FuncTraits<T>::kind == CallableKind::Class
		&& FuncTraits<T>::qualifiers.test(FuncQualifier::Const)));

/// @todo FIXME: Handle inherited functions, getters returing proxy objects, implicit/explicit
/// conversions in setters, "deducing this" functions, non-const getters, ...
template<auto G, auto S>
concept c_getter_setter_pair
	= FuncTraits<decltype(G)>::kind == CallableKind::MemberFunction
	&& FuncTraits<decltype(S)>::kind == CallableKind::MemberFunction
	&& std::same_as<typename FuncTraits<decltype(G)>::ClassType,
		typename FuncTraits<decltype(G)>::ClassType>
	&& requires(typename FuncTraits<decltype(G)>::ClassType x) {
		(x.*S)((x.*G)());
	};

} // namespace fr
#endif // include guard
