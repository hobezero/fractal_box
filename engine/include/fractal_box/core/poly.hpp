#ifndef FRACTAL_BOX_CORE_POLY_HPP
#define FRACTAL_BOX_CORE_POLY_HPP

/// @file
/// @brief Utilities for polymorphic types including a limited reimplementation
/// of std::dynamic_cast that doesn't require RTTI
// TODO: Add customisation points to poly::is
// TODO: Implement poly::cast and poly::dyn_cast overloads that take references
// TODO: Add support for smart pointers (std::unique_ptr, custom types, ...)
// TODO: Implement an optional mechanism for computing type ids based on adress of a static variable
// TODO: Implement an optional mechanism for computing type ids based on hash of class name,
// but the name is derived automatically using compiler-specific macros

#include <type_traits>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/type_name.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr::poly {

/// @brief Unique identifier of a class. By default uses 64-bit hash value of the class name.
/// Works across .dll and .so boundaries
/// @note While the probability of it happening is extremely low, there is a chance of collision
/// between hashes of two classes.
using ClassId = HashDigest64;

template<typename To, typename From>
[[nodiscard]] inline
auto is(const From& obj) noexcept -> bool {
	static_assert(!std::is_pointer_v<To> && !std::is_reference_v<To>);
	if constexpr (std::is_base_of_v<To, From>) {
		// Upcast is trivial
		return true;
	}
	else {
		static_assert(std::is_base_of_v<From, To>);
		return obj.is_derived_from(To::class_id);
	}
}

template<typename To, typename From>
[[nodiscard]] inline
auto cast(const From& obj) noexcept -> To {
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline
auto cast(From& obj) noexcept -> To {
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline
auto cast(const From* obj) noexcept -> To {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(*obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline
auto cast(From* obj) noexcept -> To {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(*obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline
auto dyn_cast(const From* obj) noexcept -> To {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	return is<SimpleTo>(*obj) ? static_cast<To>(obj) : nullptr;
}

template<typename To, typename From>
[[nodiscard]] inline
auto dyn_cast(From* obj) noexcept -> To {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	return is<SimpleTo>(*obj) ? static_cast<To>(obj) : nullptr;
}

namespace detail {

inline consteval
auto is_identifier_fully_qualified(const char* name, size_t len) noexcept -> bool {
	return len >= 2 && name[0] == ':' && name[1] == ':';
}

inline consteval
auto is_identifier_a_template(const char* name, size_t len) noexcept -> bool {
	for (std::size_t i = 0; i < len; ++i) {
		if (name[i] == '<')
			return true;
	}
	return false;
}

} // namespace detail
} // namespace fr::poly

#define FR_POLY_DEFINE_ROOT() \
	virtual bool is_derived_from(::fr::poly::ClassId) const noexcept { return false; }

// TODO: What about anonymous namespaces?
/// @brief Define class_id for derived. Define is_derived_from function to match the class against
/// any class_id at runtime
/// @note ClassId based on names is susceptible to ambiguous identifiers since the same
/// class might be spelled in several ways and miltiple classes might have the same unqalified name.
/// Because of this use fully qualified names (e.g., ::myapp::mycomponent::MyClass<Param>)
/// @warning Doesn't work with templates. Doesn't work when inherited from multiple polymorphic
/// classes with defined class_id
#define FR_POLY_DEFINE_ID(derived, base) \
	static constexpr ::fr::poly::ClassId class_id = fr::type_hash64<derived>; \
	auto is_derived_from(::fr::poly::ClassId base_id) const noexcept -> bool  override { \
		static_assert(::std::is_same_v<const derived*, decltype(this)>); \
		static_assert(::fr::poly::detail::is_identifier_fully_qualified(FR_TO_STRING(derived), \
			FR_LITERAL_STRLEN(derived)), "Class name " #derived " should be fully qualified"); \
		static_assert(!::fr::poly::detail::is_identifier_a_template(FR_TO_STRING(derived), \
			FR_LITERAL_STRLEN(derived)), "Invalid class name " #derived \
			". Templates are not supported"); \
		static_assert(::std::is_base_of_v<base, derived> && !::std::is_same_v<base, derived>, \
			#base " is not a base class of " #derived); \
		return base_id == class_id || base::is_derived_from(base_id); \
	}

#endif // include guard
