#ifndef FRACTAL_BOX_CORE_STRING_LITERAL_HPP
#define FRACTAL_BOX_CORE_STRING_LITERAL_HPP

#include <string_view>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

namespace fr {

/// @see P3094: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3094r6.html
template<c_character T, size_t Size>
requires (Size > 0)
struct StringLiteral {
	using ValueType = T;
	using StringViewType = std::basic_string_view<T>;

	static constexpr auto size = SizeC<Size>{};

	explicit(false) constexpr
	StringLiteral(StringViewType str) noexcept {
		FR_ASSERT(str.size() <= Size);
		for (auto i = 0uz; i < Size; ++i)
			_data[i] = str[i];
		_data[Size] = T{};
	}

	explicit(false) constexpr
	StringLiteral(const T* str) noexcept {
		for (auto i = 0uz; i < Size; ++i)
			_data[i] = str[i];
		_data[Size] = T{};
	}

	template<size_t OtherSize>
	constexpr
	auto operator==(const StringLiteral<T, OtherSize>& other) const noexcept -> bool {
		if constexpr (Size != OtherSize)
			return false;
		return view() == other.view();
	}

	template<size_t OtherSize>
	constexpr
	auto operator<=>(const StringLiteral<T, OtherSize>& other) const noexcept {
		return view() <=> other.view();
	}

	constexpr
	auto operator==(StringViewType other) const noexcept -> bool {
		return view() == other;
	}

	constexpr
	auto operator<=>(StringViewType other) const noexcept {
		return view() <=> other;
	}

	explicit(false) constexpr
	operator StringViewType() const noexcept { return {_data, Size}; }

	constexpr
	auto view() const noexcept -> StringViewType { return {_data, Size}; }

	constexpr
	auto operator[](size_t i) const noexcept -> const T& { return _data[i]; }

	constexpr
	auto operator[](size_t i) noexcept -> T& { return _data[i]; }

	constexpr
	auto cbegin() const noexcept -> const T* { return _data; }

	constexpr
	auto begin() const noexcept -> const T* { return _data; }

	constexpr
	auto begin() noexcept -> T* { return _data; }

	constexpr
	auto cend() const noexcept -> const T* { return _data + Size; }

	constexpr
	auto end() const noexcept -> const T* { return _data + Size; }

	constexpr
	auto end() noexcept -> T* { return _data + Size; }

	constexpr
	auto data() const noexcept -> const T* { return _data; }

	constexpr
	auto data() noexcept -> T* { return _data; }

public: // Can't have private data members in a structural type
	T _data[Size + 1zu];
};

template<class T, size_t ArrSize>
StringLiteral(const T (&str)[ArrSize]) -> StringLiteral<T, ArrSize - 1zu>;

} // namespace fr
#endif // include guard
