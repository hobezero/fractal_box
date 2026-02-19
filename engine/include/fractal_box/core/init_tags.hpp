#ifndef FRACTAL_BOX_CORE_INIT_TAGS_HPP
#define FRACTAL_BOX_CORE_INIT_TAGS_HPP

#include <type_traits>

namespace fr {

/// @brief Tag type used to indicate that an overloaded constructor or a factory function will NOT
/// initialize data members
/// @warning Inherently unsafe, use only if performance penalty of zero initialization or value
/// initialization is unacceptable. `UninitializedInit` can also be used to intentionally trigger
/// UBSAN and/or static analyzers
struct UninitializedInit {
	explicit
	UninitializedInit() = default;
};

inline constexpr auto uninitialized = UninitializedInit{};

/// @brief Tag type used to indicate that an overloaded constructor or a factory function will zero
/// initialize data members
struct ZeroInit {
	explicit
	ZeroInit() = default;
};

inline constexpr auto zero_init = ZeroInit{};

/// @brief Tag type used to indicate that an overloaded constructor or a factory function will
/// initialize data members by taking ownership of resource(s) as-is, without performing any
/// potentially fallible operations
/// @warning Inherently unsafe, as it is not usually possible to check if a resource handle
/// refers to a resource within its lifetime bounds, and no other objects own this resource
struct AdoptInit {
	explicit
	AdoptInit() = default;
};

inline constexpr auto adopt = AdoptInit{};

/// @brief Tag type used to inidicate that an overloaded constructor or a factory functions of a
/// wrapper type should constsruct a contained object in-place. Equivalent to `std::in_place`
struct InPlaceInit {
	explicit
	InPlaceInit() = default;
};

inline constexpr auto in_place = InPlaceInit{};

/// @brief Tag type used to indicate that an overloaded constructor or a factory function will
/// construct an empty object
struct EmptyInit {
	explicit
	EmptyInit() = default;
};

inline constexpr auto empty_init = EmptyInit{};

struct NewInit {
	explicit
	NewInit() = default;
};

inline constexpr auto new_init = NewInit{};

struct NextInit {
	explicit
	NextInit() = default;
};

inline constexpr auto next_init = NextInit{};

/// @brief Tag type used to inidicate that an overloaded constructor or a factory function of a
/// wrapper type should constsruct a contained object of type `T` in-place. Equivalent to
/// `std::in_place_type_t`
template<class T>
struct InPlaceAsInit {
	explicit
	InPlaceAsInit() = default;
};

template<class T>
inline constexpr auto in_place_as = InPlaceAsInit<T>{};

namespace detail {

template<class T>
inline constexpr auto is_in_place_as_init = false;

template<class T>
inline constexpr auto is_in_place_as_init<InPlaceAsInit<T>> = true;

} // namespace detail

template<class T>
concept is_in_place_as_init = detail::is_in_place_as_init<std::remove_cvref_t<T>>;

/// @brief Tag type used to disambiguate a constructor or a factory function that
/// takes an `std::initializer_list` from other overloads
struct FromListInit {
	explicit
	FromListInit() = default;
};

inline constexpr auto from_list = FromListInit{};

/// @brief Tag type used to disambiguate a constructor or a factory function that
/// takes an `std::ranges::range`-like sequence of elements from other overloads. Equivalent to
/// `std::from_range_t`
struct FromRangeInit {
	explicit
	FromRangeInit() = default;
};

inline constexpr auto from_range = FromRangeInit{};

template<class SizeType>
struct FromCapacity {
	explicit constexpr
	FromCapacity(SizeType val) noexcept: value{val} { }

public:
	SizeType value;
};

template<class T>
FromCapacity(const T&) -> FromCapacity<T>;

} // namespace fr
#endif // include guard
