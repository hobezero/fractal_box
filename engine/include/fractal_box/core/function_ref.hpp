#ifndef FRACTAL_BOX_CORE_FUNCTION_REF_HPP
#define FRACTAL_BOX_CORE_FUNCTION_REF_HPP

#include <functional>
#include <utility>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

namespace fr {

template<auto V>
struct NonType
{
	explicit
	NonType() = default;
};

template<auto V>
inline constexpr auto nontype = NonType<V>{};

template<class T>
inline constexpr auto is_nontype = false;

template<auto F>
inline constexpr auto is_nontype<NonType<F>> = true;

template<class T>
concept c_nontype = is_nontype<T>;

namespace detail {

// See also: https://www.agner.org/optimize/calling_conventions.pdf
template<class T>
using FuncRefParam = typename MpLazyIf<std::is_trivially_copyable_v<T>>::template Type<
	T,
	std::add_rvalue_reference_t<T>
>;

template<class T, class Self>
inline constexpr auto is_not_self = !std::is_same_v<std::remove_cvref_t<T>, Self>;

template<class T, template<class...> class>
inline constexpr auto looks_nullable_to_impl = std::is_member_pointer_v<T>;

template<class F, template<class...> class Self>
inline constexpr auto looks_nullable_to_impl<F *, Self> = std::is_function_v<F>;

template<class... S, template<class...> class Self>
inline constexpr auto looks_nullable_to_impl<Self<S...>, Self> = true;

template<class S, template<class...> class Self>
inline constexpr auto looks_nullable_to = looks_nullable_to_impl<std::remove_cvref_t<S>, Self>;

template<class T>
struct AdaptSignatureImpl;

template<c_function F>
struct AdaptSignatureImpl<F*> {
	using Type = F;
};

template<class Fp>
using AdaptSignature = AdaptSignatureImpl<Fp>::Type;

template<class S>
struct NotQualifyingThis
{};

template<class R, class... Args>
struct NotQualifyingThis<R(Args...)>
{
	using Type = R(Args...);
};

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) noexcept> {
	using Type = R(Args...) noexcept;
};

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) &>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const &>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile &>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile &>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) &&>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const &&>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile &&>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile &&>:
	NotQualifyingThis<R(Args...)>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) & noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const & noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile & noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile & noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) && noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const && noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) volatile && noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class R, class... Args>
struct NotQualifyingThis<R(Args...) const volatile && noexcept>:
	NotQualifyingThis<R(Args...) noexcept>
{ };

template<class F, class T>
struct DropFirstArgToInvokeImpl;

template<class T, class R, class G, class... Args>
struct DropFirstArgToInvokeImpl<R (*)(G, Args...), T> {
	using Type = R(Args...);
};

template<class T, class R, class G, class... Args>
struct DropFirstArgToInvokeImpl<R (*)(G, Args...) noexcept, T> {
	using Type = R(Args...) noexcept;
};

template<class T, c_object M, class G>
struct DropFirstArgToInvokeImpl<M G::*, T> {
	using Type = std::invoke_result_t<M G::*, T>() noexcept;
};

template<class T, c_function M, class G>
struct DropFirstArgToInvokeImpl<M G::*, T>: NotQualifyingThis<M> { };

template<class F, class T>
using DropFirstArgToInvoke = DropFirstArgToInvokeImpl<F, T>::Type;

template<class Sig>
struct QualFnSig;

template<class R, class... Args>
struct QualFnSig<R(Args...)>
{
	using Function = R(Args...);
	using WithoutNoexcept = Function;
	static constexpr bool is_noexcept = false;

	template<class... T>
	static constexpr bool is_invocable_using = std::is_invocable_r_v<R, T..., Args...>;

	template<class T>
	using Cv = T;
};

template<class R, class... Args>
struct QualFnSig<R(Args...) noexcept>
{
	using Function = R(Args...);
	using WithoutNoexcept = Function;
	static constexpr bool is_noexcept = true;

	template<class... T>
	static constexpr bool is_invocable_using = std::is_nothrow_invocable_r_v<R, T..., Args...>;

	template<class T>
	using Cv = T;
};

template<class R, class... Args>
struct QualFnSig<R(Args...) const>: QualFnSig<R(Args...)>
{
	template<class T>
	using Cv = const T;

	using WithoutNoexcept = R(Args...) const;
};

template<class R, class... Args>
struct QualFnSig<R(Args...) const noexcept>: QualFnSig<R(Args...) noexcept>
{
	template<class T>
	using Cv = const T;

	using WithoutNoexcept = R(Args...) const;
};

struct FunctionRefBase {
	union Storage {
		Storage() = default;

		template<c_object T>
		explicit constexpr
		Storage(T* p) noexcept: ptr{p} { }

		template<c_object T>
		explicit constexpr
		Storage(T const* p) noexcept: const_ptr{p} { }

		template<c_function T>
		explicit constexpr
		Storage(T* p) noexcept: func_ptr{reinterpret_cast<decltype(func_ptr)>(p)} { }

	public:
		void* ptr = nullptr;
		void const* const_ptr;
		void (*func_ptr)();
	};

	template<class T>
	constexpr static
	auto get(Storage obj)
	{
		if constexpr (std::is_const_v<T>)
			return static_cast<T*>(obj.const_ptr);
		else if constexpr (std::is_object_v<T>)
			return static_cast<T*>(obj.ptr);
		else
			return reinterpret_cast<T*>(obj.func_ptr);
	}
};

} // namespace detail

template<class Sig, class = typename detail::QualFnSig<Sig>::Function>
class FunctionRef;

template<class From, class To>
inline constexpr bool is_ref_convertible = false;

template<class T, class U>
inline constexpr bool is_ref_convertible<FunctionRef<T>, FunctionRef<U>> =
	std::is_convertible_v<
		typename detail::NotQualifyingThis<T>::Type &,
		typename detail::NotQualifyingThis<U>::Type &
	>;

template<class Sig, class R, class... Args>
class FunctionRef<Sig, R(Args...)>: detail::FunctionRefBase
{
	using Signature = detail::QualFnSig<Sig>;

	template<class T>
	using Cv = Signature::template Cv<T>;

	template<class T>
	using CvRef = Cv<T>&;

	static constexpr bool noex = Signature::is_noexcept;

	template<class... T>
	static constexpr bool is_invocable_using = Signature::template is_invocable_using<T...>;

	template<class F>
	static constexpr bool is_convertible_from_specialization =
		is_ref_convertible<F, FunctionRef>;

	using Thunk = R (Storage, detail::FuncRefParam<Args>...) noexcept(noex);

	friend
	class FunctionRef<typename Signature::WithoutNoexcept>;

public:
	template<class F>
	explicit(false)
	FunctionRef(F* f) noexcept
	requires std::is_function_v<F> && is_invocable_using<F>:
		_thunk{[](Storage fn_, detail::FuncRefParam<Args>... args) noexcept(noex) -> R {
			if constexpr (std::is_void_v<R>)
				get<F>(fn_)(static_cast<decltype(args)>(args)...);
			else
				return get<F>(fn_)(static_cast<decltype(args)>(args)...);
		}},
		_obj{f}
	{
		FR_ASSERT_MSG(f != nullptr, "Must reference a function");
	}

	template<class F, class T = std::remove_reference_t<F>>
	explicit(false) constexpr
	FunctionRef(F&& f) noexcept
	requires (!is_convertible_from_specialization<std::remove_cv_t<T>> &&
		!std::is_member_pointer_v<T> &&
		is_invocable_using<CvRef<T>>
	):
		_thunk{[](Storage fn_, detail::FuncRefParam<Args>... args) noexcept(noex) -> R {
			CvRef<T> obj = *get<T>(fn_);
			if constexpr (std::is_void_v<R>)
				obj(static_cast<decltype(args)>(args)...);
			else
				return obj(static_cast<decltype(args)>(args)...);
		}},
		_obj{std::addressof(f)}
	{ }

	template<class F>
	explicit(false) constexpr
	FunctionRef(F f) noexcept
	requires (!std::same_as<F, FunctionRef> && is_convertible_from_specialization<F>):
		_thunk{f.fptr_},
		_obj{f.obj_}
	{ }

	template<class T>
	auto operator=(T) -> FunctionRef&
	requires(
		!is_convertible_from_specialization<T> && !std::is_pointer_v<T> && !c_nontype<T>
	) = delete;

	template<auto F>
	explicit(false) constexpr
	FunctionRef(NonType<F>) noexcept
	requires is_invocable_using<decltype((F))>:
		_thunk{[](Storage, detail::FuncRefParam<Args>... args) noexcept(noex) -> R {
			return std::invoke_r<R>(F, static_cast<decltype(args)>(args)...);
		}}
	{
		using FT = decltype(F);
		if constexpr (std::is_pointer_v<FT> || std::is_member_pointer_v<FT>)
			static_assert(F != nullptr, "NTTP callable must be usable");
	}

	template<auto F, class U, class T = std::remove_reference_t<U>>
	constexpr
	FunctionRef(NonType<F>, U&& obj) noexcept
	requires (!std::is_rvalue_reference_v<U&&> && is_invocable_using<decltype((F)), CvRef<T>>):
		_thunk{[](Storage this_, detail::FuncRefParam<Args>... args) noexcept(noex) -> R {
			CvRef<T> obj = *get<T>(this_);
			return std::invoke_r<R>(F, obj, static_cast<decltype(args)>(args)...);
		}},
		_obj{std::addressof(obj)}
	{
		using FT = decltype(F);
		if constexpr (std::is_pointer_v<FT> || std::is_member_pointer_v<FT>)
			static_assert(F != nullptr, "NTTP callable must be usable");
	}

	template<auto F, class T>
	constexpr
	FunctionRef(NonType<F>, Cv<T> *obj) noexcept
	requires is_invocable_using<decltype((F)), decltype(obj)>:
		_thunk{[](Storage this_, detail::FuncRefParam<Args>... args) noexcept(noex) -> R {
			return std::invoke_r<R>(F, get<Cv<T>>(this_), static_cast<decltype(args)>(args)...);
		}},
		_obj{obj}
	{
		using FT = decltype(F);
		if constexpr (std::is_pointer_v<FT> || std::is_member_pointer_v<FT>)
			static_assert(F != nullptr, "NTTP callable must be usable");

		if constexpr (std::is_member_pointer_v<FT>)
			FR_ASSERT_MSG(obj != nullptr, "Must reference an object");
	}

	constexpr
	auto operator()(Args... args) const noexcept(noex) -> R {
		return _thunk(_obj, std::forward<Args>(args)...);
	}

private:
	Thunk* _thunk = nullptr;
	Storage _obj;
};

template<c_function F>
FunctionRef(F*) -> FunctionRef<F>;

template<auto V>
FunctionRef(NonType<V>) -> FunctionRef<detail::AdaptSignature<decltype(V)>>;

template<auto V, class T>
FunctionRef(NonType<V>, T&&) -> FunctionRef<detail::DropFirstArgToInvoke<decltype(V), T&>>;

} // namespace fr
#endif // include guardd
