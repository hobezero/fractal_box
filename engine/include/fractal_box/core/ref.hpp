#ifndef FRACTAL_BOX_CORE_REF_HPP
#define FRACTAL_BOX_CORE_REF_HPP

#include <cstddef>

#include <memory>
#include <type_traits>

#include "fractal_box/core/platform.hpp"

namespace fr {

/// @brief Non-owning non-nullable reference wrapper
/// @details
/// `Ref` is a simple class. Unlike `std::reference_wrapper`, `Ref` doesn't support `operator()`,
/// doesn't need factory functions (replaced with deduction guides), doesn't include `<functional>`
/// @todo TODO: Specialize `std::basic_common_reference`
template<class T>
class Ref {
	static FR_FORCE_INLINE constexpr
	auto to_ptr(T& obj) noexcept -> T* {
		return std::addressof(obj);
	}

	static
	void to_ptr(T&& obj) = delete;

public:
	using Type = T;

	Ref() = delete;

	/// @brief Construct Ref from a compatible reference
	/// @note See LWG 2993, LWG 3041 for more details on the design
	template<class U>
	requires (!std::same_as<Ref, std::remove_cv_t<U>> && requires(U u) { Ref::to_ptr(u); })
	FR_FORCE_INLINE constexpr
	Ref(U&& obj) noexcept(noexcept(Ref::to_ptr(std::declval<U>()))):
		_ptr(Ref::to_ptr(std::forward<U>(obj)))
	{ }

	FR_FORCE_INLINE constexpr
	operator T&() const noexcept { return *_ptr; }

	FR_FORCE_INLINE constexpr
	auto get() const noexcept -> T& { return *_ptr; }

	FR_FORCE_INLINE constexpr
	auto operator*() const noexcept -> T& { return *_ptr; }

	FR_FORCE_INLINE constexpr
	auto operator->() const noexcept -> T* { return _ptr; }

private:
	T* _ptr;
};

template<class T>
Ref(T&) -> Ref<T>;

struct AsRef {
	template<class T>
	FR_FORCE_INLINE
	auto operator()(T&& obj) const noexcept(noexcept(Ref{std::forward<T>(obj)})) {
		return Ref{std::forward<T>(obj)};
	}
};

inline constexpr auto as_ref = AsRef{};

} // namespace fr
#endif // include guard
