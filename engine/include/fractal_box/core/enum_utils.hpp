#ifndef FRACTAL_BOX_CORE_FLAGS_HPP
#define FRACTAL_BOX_CORE_FLAGS_HPP

#include <bit>
#include <compare>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @brief Convert enum to its underlying integral-type value. Equivalent to `std::to_underlying`
/// from C++23
/// @note Not superseded by `std::underlying` because the standard library version still generates
/// a funtion call in debug mode
template<c_enum E>
FR_FORCE_INLINE constexpr
auto to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}

/// @todo
///   TODO: Apply CRTP to support inheriting from `Flags`. This would allow user to define
///   named combinations of flags in their own class
///   TODO: Look into https://andreasfertig.blog/2024/01/cpp20-concepts-applied/
template<c_enum FlagEnum>
class Flags {
public:
	using EnumType = FlagEnum;
	using UnderlyingType = std::underlying_type_t<FlagEnum>;
	using StorageType = std::make_unsigned_t<UnderlyingType>;

	Flags() = default;

	FR_FORCE_INLINE explicit constexpr
	Flags(AdoptInit, StorageType value) noexcept:
		_value{value}
	{ }

	FR_FORCE_INLINE explicit(false) constexpr
	Flags(FlagEnum flag) noexcept:
		_value{std::bit_cast<StorageType>(flag)}
	{ }

	/// @brief Construct Flags from a sequence of flags
	template<size_t N>
	constexpr explicit(false)
	Flags(const FlagEnum (&flags)[N]) noexcept:
		_value{make_raw_union(flags)}
	{ }

	/// @brief Construct Flags from a sequence of flags
	/// @note One of the rare cases where using `std::initializer_list` is acceptable
	constexpr
	Flags(FromListInit, std::initializer_list<FlagEnum> flags) noexcept:
		_value{make_raw_union(flags)}
	{ }

	/// @brief Construct Flags from a sequence of flags
	template<class Range>
	constexpr Flags(FromRangeInit, Range&& flags) noexcept
	// Poor man's `std::range_value_t` which doesn't require the <ranges> header
	requires std::convertible_to<decltype(*std::begin(flags)), FlagEnum>:
		_value{make_raw_union(std::forward<Range>(flags))}
	{ }

	friend
	constexpr void swap(Flags &lhs, Flags &rhs) noexcept {
		using std::swap;
		swap(lhs._value, rhs._value);
	}

	friend
	auto operator==(Flags lhs, Flags rhs) -> bool = default;

	/// @attention Because of the implicit constructor from `FlagEnum` comparing `Flags`
	/// with `FlagEnum` (e.g. `Flags{FlagEnum::A} == FlagEnum::A`) would be legal unless explicitly
	/// banned. The idea is to prevent potential logic bugs where a set of flags is intended to be
	/// tested against another set of flags, but a a single flag is passed instead
	/// @note Use `test(..)` member function to compare `Flags` with `FlagEnum`
	auto operator==(FlagEnum rhs) -> bool = delete;

	constexpr
	auto raw_value() const noexcept -> StorageType {
		return _value;
	}

	constexpr
	auto test(FlagEnum flag) const noexcept -> bool {
		return (_value & std::bit_cast<StorageType>(flag)) != StorageType{0};
	}

	constexpr
	auto test_all_of(Flags flags) const noexcept -> bool {
		return (_value & flags._value) == flags._value;
	}

	constexpr
	auto test_none_of(Flags flags) const noexcept -> bool {
		return (_value & flags._value) == StorageType{0};
	}

	constexpr
	auto test_any_of(Flags flags) const noexcept -> bool {
		return (_value & flags._value) != StorageType{0};
	}

	/// @post `this->test(flag) == on`
	constexpr
	auto set(FlagEnum flag, bool on) noexcept -> Flags& {
		// TODO: Investigate the performance of the branching vs branch-free versions
		_value &= ~std::bit_cast<StorageType>(flag);
		_value |= on * std::bit_cast<StorageType>(flag);
		return *this;
	}

	/// @post `this->test(flags) == on`
	constexpr
	auto set(Flags flags, bool on) noexcept -> Flags& {
		_value &= ~flags._value;
		_value |= on * flags._value;
		return *this;
	}

	/// @brief Equivalent to `this->set(flag, true)`
	/// @post `this->test(flag)`
	constexpr
	auto set(FlagEnum flag) noexcept -> Flags& {
		_value |= std::bit_cast<StorageType>(flag);
		return *this;
	}

	/// @brief Equivalent to `this->set(flags, true)`
	/// @post `this->test(flags)`
	constexpr
	auto set(Flags flags) noexcept -> Flags& {
		_value |= flags._value;
		return *this;
	}

	/// @brief Equivalent to `this->set(flag, false)`
	/// @post `!this->test(flag)`
	constexpr
	auto reset(FlagEnum flag) noexcept -> Flags& {
		_value &= ~std::bit_cast<StorageType>(flag);
		return *this;
	}

	/// @brief Equivalent to `this->set(flags, false)`
	/// @post `!this->test(flags)`
	constexpr
	auto reset(Flags flags) noexcept -> Flags& {
		_value &= ~flags._value;
		return *this;
	}

	constexpr
	auto flip(FlagEnum flag) noexcept -> Flags& {
		_value ^= std::bit_cast<StorageType>(flag);
		return *this;
	}

	constexpr
	auto flip(Flags flags) noexcept -> Flags& {
		_value ^= flags._value;
		return *this;
	}

	/// @post `this->empty()`
	constexpr
	auto clear() noexcept -> Flags& {
		_value = {};
		return *this;
	}

	constexpr
	explicit operator bool() const noexcept {
		return _value;
	}

	[[nodiscard]] constexpr
	auto empty() const noexcept -> bool {
		return !_value;
	}

	constexpr
	auto count() const noexcept -> int
	requires requires(StorageType v) { std::popcount(v); } {
		return std::popcount(_value);
	}

	// Bitwise operators

	constexpr
	auto operator|=(Flags other) noexcept -> Flags& {
		_value |= other._value;
		return *this;
	}

	constexpr
	auto operator|=(FlagEnum other) noexcept -> Flags& {
		_value |= static_cast<StorageType>(other);
		return *this;
	}

	constexpr
	auto operator&=(Flags other) noexcept -> Flags& {
		_value &= other._value;
		return *this;
	}

	constexpr
	auto operator&=(FlagEnum other) noexcept -> Flags& {
		_value &= static_cast<StorageType>(other);
		return *this;
	}

	constexpr
	auto operator^=(Flags other) noexcept -> Flags& {
		_value ^= other._value;
		return *this;
	}

	constexpr
	auto operator^=(FlagEnum other) noexcept -> Flags& {
		_value ^= static_cast<StorageType>(other);
		return *this;
	}

	// TODO: Implement operator~

	friend constexpr
	auto operator|(Flags lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value | rhs._value};
	}

	friend constexpr
	auto operator|(Flags lhs, FlagEnum rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value | static_cast<StorageType>(rhs)};
	}

	friend constexpr
	auto operator|(FlagEnum lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, static_cast<StorageType>(lhs) | rhs._value};
	}

	friend constexpr
	auto operator&(Flags lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value & rhs._value};
	}

	friend constexpr
	auto operator&(Flags lhs, FlagEnum rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value & static_cast<StorageType>(rhs)};
	}

	friend constexpr
	auto operator&(FlagEnum lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, static_cast<StorageType>(lhs._value) & rhs._value};
	}

	friend constexpr
	auto operator^(Flags lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value ^ rhs._value};
	}

	friend constexpr
	auto operator^(Flags lhs, FlagEnum rhs) noexcept -> Flags {
		return Flags{adopt, lhs._value ^ static_cast<StorageType>(rhs)};
	}

	friend constexpr
	auto operator^(FlagEnum lhs, Flags rhs) noexcept -> Flags {
		return Flags{adopt, static_cast<StorageType>(lhs) ^ rhs._value};
	}

private:
	template<class R>
	FR_FORCE_INLINE static constexpr
	auto make_raw_union(R&& flags) noexcept -> StorageType {
		auto result = StorageType{};
		for (const auto& flag : std::forward<R>(flags))
			result |= std::bit_cast<StorageType>(static_cast<FlagEnum>(flag));
		return result;
	}

private:
	StorageType _value {0};
};

} // namespace fr
#endif // FR_WARS_FLAGS_HPP
