#ifndef FRACTAL_BOX_CORE_POLY_HPP
#define FRACTAL_BOX_CORE_POLY_HPP

/// @file
/// @brief Utilities for polymorphic types including a limited reimplementation
/// of std::dynamic_cast that doesn't require RTTI
// TODO: Add customisation points to poly::is
// TODO: Implement poly::cast and poly::dynCast overloads that take references
// TODO: Add support for smart pointers (std::unique_ptr, custom types, ...)
// TODO: Implement an optional mechanism for computing type ids based on adress of a static variable
// TODO: Implement an optional mechanism for computing type ids based on hash of class name,
// but the name is derived automatically using compiler-specific macros

#include <type_traits>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/hashing/fnv.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr::poly {

/// @brief Unique identifier of a class. By default uses 64-bit hash value of the class name.
/// Works across .dll and .so boundaries
/// @note While the probability of it happening is extremely low, there is a chance of collision
/// between hashes of two classes.
using ClassId = std::uint64_t;

template<typename To, typename From>
[[nodiscard]] inline bool is(const From& obj) noexcept {
	static_assert(!std::is_pointer_v<To> && !std::is_reference_v<To>);
	if constexpr (std::is_base_of_v<To, From>) {
		// Upcast is trivial
		return true;
	}
	else {
		static_assert(std::is_base_of_v<From, To>);
		return obj.isDerivedFrom(To::classId);
	}
}

template<typename To, typename From>
[[nodiscard]] inline To cast(const From& obj) noexcept {
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline To cast(From& obj) noexcept {
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline To cast(const From* obj) noexcept {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline To cast(From* obj) noexcept {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	FR_ASSERT(is<SimpleTo>(obj));
	return static_cast<To>(obj);
}

template<typename To, typename From>
[[nodiscard]] inline To dynCast(const From* obj) noexcept {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	return is<SimpleTo>(*obj) ? static_cast<To>(obj) : nullptr;
}

template<typename To, typename From>
[[nodiscard]] inline To dynCast(From* obj) noexcept {
	FR_ASSERT(obj);
	using SimpleTo = std::remove_pointer_t<std::decay_t<To>>;
	return is<SimpleTo>(*obj) ? static_cast<To>(obj) : nullptr;
}

namespace detail {

inline constexpr bool isIdentifierQualified(const char *name, std::size_t len) noexcept {
	return len >= 2 && name[0] == ':' && name[1] == ':';
}

inline constexpr bool isIdentifierATemplate(const char *name, std::size_t len) noexcept {
	for (std::size_t i = 0; i < len; ++i) {
		if (name[i] == '<')
			return true;
	}
	return false;
}

} // namespace detail
} // namespace fr::poly

#define FR_POLY_DEFINE_ROOT() \
	virtual bool isDerivedFrom(::fr::poly::ClassId) const noexcept { return false; }

// TODO: What about anonymous namespaces?
/// @brief Define classId for derived. Define isDerivedFrom function to match the class against
/// any classId at runtime
/// @note ClassId based on names is susceptible to ambiguous identifiers since the same
/// class might be spelled in several ways and miltiple classes might have the same unqalified name.
/// Because of this use fully qualified names (e.g., ::myapp::mycomponent::MyClass<Param>)
/// @warning Doesn't work with templates. Doesn't work when inherited from multiple polymorphic
/// classes with defined classId
#define FR_POLY_DEFINE_ID(derived, base) \
	static constexpr ::fr::poly::ClassId classId{ \
		::fr::fnv1a_hash_string<::fr::poly::ClassId>( \
			FR_TO_STRING(derived), FR_LITERAL_STRLEN(derived) \
		) \
	}; \
	bool isDerivedFrom(::fr::poly::ClassId baseId) const noexcept override { \
		static_assert(::std::is_same_v<const derived*, decltype(this)>); \
		static_assert(::fr::poly::detail::isIdentifierQualified(FR_TO_STRING(derived), \
			FR_LITERAL_STRLEN(derived)), "Class name " #derived " should be qualified"); \
		static_assert(!::fr::poly::detail::isIdentifierATemplate(FR_TO_STRING(derived), \
			FR_LITERAL_STRLEN(derived)), "Invalid class name " #derived \
			". Templates are not supported"); \
		static_assert(::std::is_base_of_v<base, derived> && !::std::is_same_v<base, derived>, \
			#base " is not a base class of " #derived); \
		return baseId == classId || base::isDerivedFrom(baseId); \
	}

#endif // include guard
