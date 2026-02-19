#ifndef FRACTAL_BOX_CORE_TYPE_INDEX_HPP
#define FRACTAL_BOX_CORE_TYPE_INDEX_HPP

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/description_types.hpp"
#include "fractal_box/core/hashing/hashing_attributes.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T>
concept c_type_index_domain
	= std::integral<typename T::ValueType>
	&& (!requires { T::init_value; }
		|| std::convertible_to<std::remove_cv_t<decltype(T::init_value)>, typename T::ValueType>)
	&& (!requires { T::null_value; }
		|| std::convertible_to<std::remove_cv_t<decltype(T::null_value)>, typename T::ValueType>)
	&& (!requires { T::init_value; T::null_value; } || T::init_value != T::null_value)
;

struct DefaultTypeIndexDomain {
	using ValueType = uint32_t;
	static constexpr ValueType init_value = 0;
};

static_assert(c_type_index_domain<DefaultTypeIndexDomain>);

template<class V = DefaultTypeIndexDomain::ValueType, V Init = V{0}>
struct CustomTypeIndexDomainBase {
	using ValueType = V;
	static constexpr ValueType init_value = Init;
};

template<c_type_index_domain Domain = DefaultTypeIndexDomain>
class TypeIndex {
	static
	auto next_counter() noexcept {
		static auto counter = typename Domain::ValueType{init_value()};
		return counter++;
	}

public:
	using DomainType = Domain;
	using ValueType = typename Domain::ValueType;

	static constexpr auto has_init_value = requires { Domain::init_value; };

	static FR_FORCE_INLINE consteval
	auto init_value() noexcept -> ValueType {
		if constexpr (has_init_value)
			return Domain::init_value;
		else
			return 0;
	}


	static constexpr auto has_null_value = requires { Domain::null_value; };

	static FR_FORCE_INLINE consteval
	auto null_value() noexcept -> ValueType {
		if constexpr (has_null_value)
			return Domain::null_value;
	}

	template<class T>
	static inline const auto of = TypeIndex{adopt, next_counter()};

	FR_FORCE_INLINE constexpr
	TypeIndex() noexcept
	requires(has_null_value):
		_value{null_value()}
	{ }

	explicit FR_FORCE_INLINE constexpr
	TypeIndex(UninitializedInit) noexcept
	{ }

	FR_FORCE_INLINE constexpr
	TypeIndex(AdoptInit, ValueType value) noexcept:
		_value{value}
	{ }

	friend constexpr
	void swap(TypeIndex& lhs, TypeIndex& rhs) noexcept {
		using std::swap;
		swap(lhs._value, rhs._value);
	}

	auto operator<=>(this TypeIndex, TypeIndex) = default;

	friend consteval
	auto kepler_describe(TypeIndex) noexcept {
		return class_desc<
			Attributes<Hashable{}>,
			Field<&TypeIndex::_value>
		>;
	}

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept
	requires(has_null_value) {
		return _value != null_value();
	}

	FR_FORCE_INLINE constexpr
	auto value() const noexcept -> ValueType { return _value; }

	FR_FORCE_INLINE constexpr
	auto operator*() const noexcept -> ValueType { return _value; }

private:
	ValueType _value;
};

/// @brief TypeIndex object for type `T`
/// @note Const references don't need runtime initialization
template<class T, c_type_index_domain Domain = DefaultTypeIndexDomain>
inline constexpr const auto& type_index = TypeIndex<Domain>::template of<T>;

} // namespace fr
#endif // include guard
