#ifndef FRACTAL_BOX_CORE_HASHING_HASHED_STRING_HPP
#define FRACTAL_BOX_CORE_HASHING_HASHED_STRING_HPP

#include <cstring>
#include <cwchar>

#include <string>
#include <string_view>
#include <type_traits>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/hashing/fnv.hpp"
#include "fractal_box/core/hashing/hasher_visitor_base.hpp"
#include "fractal_box/core/hashing/hashing_attributes.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/description_types.hpp"
#include "fractal_box/core/platform.hpp"

// TODO: Consider providing default constructors
// TODO: Implement spaceship operators

namespace fr {
namespace detail {

template<class Char>
inline constexpr
auto naive_str_length(const Char* str) noexcept -> size_t {
	auto len = 0uz;
	for (const auto* c = str; *c != Char{0}; ++c)
		++len;
	return len;
}

} // namespace detail

template<class T>
FR_FORCE_INLINE constexpr
auto str_length(const T& str) noexcept -> size_t {
	using Char = std::remove_cvref_t<decltype(str[0])>;
	if constexpr (std::is_array_v<T>) {
		FR_ASSERT_AUDIT_MSG(str[std::size(str) - 1] == Char{'\0'},
			"String must be null-terminated");
		return std::size(str) - 1;
	}
	else if consteval {
		return detail::naive_str_length(str);
	}
	else {
		if constexpr (std::is_same_v<Char, char>)
			return std::strlen(str);
		if constexpr (std::is_same_v<Char, wchar_t>)
			return std::wcslen(str);
		else
			return detail::naive_str_length(str);
	}
}

using DefaultStrDigest = HashDigest32;

template<class Digest, class Char = char>
class BasicHashedCStrView;

template<class Digest, class Char = char>
class BasicStringId {
public:
	using DigestType = Digest;
	using CharType = Char;

	static FR_FORCE_INLINE constexpr
	auto calc_hash(const Char* str, size_t size) -> Digest {
		return fnv1a_hash_string<Digest>(str, size);
	}

	BasicStringId() = default;

	/// @brief Don't initialize hash value at all
	/// @warning Reading hash of an object constructed with `UninitializedInit` is undefined behavior
	explicit constexpr
	BasicStringId(UninitializedInit) noexcept { }

	/// @brief Initialize to zero
	explicit constexpr
	BasicStringId(ZeroInit) noexcept: _hash{} { }

	/// @warning Supplying incorrect hash value might lead to unexpected behavior
	explicit constexpr
	BasicStringId(AdoptInit, Digest hash) noexcept: _hash{hash} { }

	/// @param Null-terminated array of Char
	template<size_t N>
	explicit(false) constexpr
	BasicStringId(const Char (&str)[N]) noexcept:
		// `std::addressof` would be more generic, but we don't want to `#include <memory>` and
		// casual operator & is OK for integral types
		_hash{calc_hash(&str[0], N - 1)}
	{
		FR_ASSERT_MSG(str[N - 1] == '\0', "Character array must be null-terminated");
	}

	explicit(false) constexpr
	BasicStringId(const Char* str, size_t length) noexcept:
		_hash{calc_hash(str, length)}
	{ }

	explicit constexpr
	BasicStringId(const std::basic_string<Char>& str) noexcept:
		_hash{calc_hash(str.data(), str.size())}
	{ }

	explicit(false) constexpr
	BasicStringId(const std::basic_string_view<Char>& str) noexcept:
		_hash{calc_hash(str.data(), str.size())}
	{ }

	friend
	auto operator==(BasicStringId, BasicStringId) -> bool = default;

	friend consteval
	auto fr_describe(BasicStringId) noexcept {
		return class_desc<
			Attributes<HashableMode::OptIn>,
			Field<&BasicStringId::_hash>,
			Property<
				"digest",
				HasherVisitorBase::Digest<Digest>,
				&BasicStringId::hvb_digest,
				nullptr,
				Attributes<Hashable{}>
			>
		>;
	}

	constexpr
	auto hash() const noexcept -> Digest { return _hash; }

	explicit constexpr
	operator bool() const noexcept { return _hash != 0; }

protected:
	Digest _hash = Digest{0};

private:
	FR_FORCE_INLINE constexpr
	auto hvb_digest() const noexcept -> HasherVisitorBase::Digest<Digest> {
		return {_hash};
	}
};

using StringId32 = BasicStringId<HashDigest32, char>;
using StringId64 = BasicStringId<HashDigest64, char>;
using StringId = BasicStringId<DefaultStrDigest, char>;

template<class T, class Char>
concept c_non_owning_string
	= std::is_convertible_v<const T&, const std::basic_string_view<Char>&>
	&& !std::is_convertible_v<const T&, const Char*>;

template<class T, class Digest, class Char>
concept c_hashed_string_like
	= c_hash_digest<Digest>
	&& std::same_as<typename T::DigestType, Digest>
	&& std::same_as<typename T::CharType, Char>
	&& std::same_as<typename T::StringIdType, BasicStringId<Digest, Char>>
	&& c_implicitly_convertible_to<const T&, typename T::StringIdType>
	&& c_implicitly_convertible_to<const typename T::ValueType&, std::basic_string_view<Char>>
	&& std::equality_comparable<T>
	&& std::constructible_from<T, typename T::ValueType>
	&& std::convertible_to<typename T::IsOwning, bool>
	&& requires(T obj) {
		{ obj.hash() } -> std::same_as<typename T::DigestType>;
		{ obj.str() } -> std::convertible_to<typename T::ValueType>;
	};

/// @brief Zero-terminated hashed owning std::basic_string
/// @note Neither `BasicHashedStr` nor `BasicStringId` is polymorphic. Object slicing is intended
/// behavior
template<class Digest, class Char = char>
class BasicHashedStr: public BasicStringId<Digest, Char> {
public:
	using StringIdType = BasicStringId<Digest, Char>;
	using StringType = std::basic_string<Char>;
	using StringViewType = std::basic_string_view<Char>;
	using ValueType = StringType;
	using IsOwning = TrueC;

	BasicHashedStr() = delete;

	/// @param hash Hash of str. Must be equal to BasicHashedStrId{str}.hash()
	/// @warning Supplying incorrect hash value might lead to unexpected behavior
	explicit constexpr
	BasicHashedStr(AdoptInit, Digest hash, StringType str) noexcept:
		StringIdType{adopt, hash}, _str(std::move(str))
	{
		FR_ASSERT_AUDIT_MSG(hash == fnv1a_hash_string<Digest>(_str.data(), _str.size()),
			"Incorrect hash value");
	}

	explicit(false) constexpr
	BasicHashedStr(StringType str):
		StringIdType{str}, _str(std::move(str))
	{ }

	/// @todo TODO: Utilize `other.release()` on owning rvalues
	template<class T>
	requires (c_hashed_string_like<T, Digest, Char>
		&& !std::same_as<T, BasicHashedStr>)
	explicit constexpr
	BasicHashedStr(const T& other):
		StringIdType{adopt, other.hash()},
		_str{other.str()}
	{ }

	template<size_t N>
	explicit(false) constexpr
	BasicHashedStr(const Char (&str)[N]):
		StringIdType{str}, _str(str)
	{ }

	friend constexpr
	auto operator==(const BasicHashedStr& lhs, const BasicHashedStr& rhs) noexcept -> bool {
		FR_ASSERT_AUDIT(lhs._hash == rhs._hash && lhs._str == rhs._str);
		return lhs._hash == rhs._hash;
	}

	friend consteval
	auto fr_describe(const BasicHashedStr&) noexcept {
		return class_desc<
			Attributes<HashableMode::OptIn>,
			Bases<StringIdType>,
			Field<&BasicHashedStr::_str>
		>;
	}

	constexpr
	auto str() const noexcept -> const StringType& {
		return _str;
	}

	[[nodiscard]] constexpr
	auto release() noexcept -> StringType {
		StringIdType::_hash = Digest{0};
		return std::move(_str);
	}

private:
	StringType _str;
};

using HashedStr32 = BasicHashedStr<HashDigest32, char>;
using HashedStr64 = BasicHashedStr<HashDigest64, char>;
using HashedStr = BasicHashedStr<DefaultStrDigest, char>;

static_assert(c_hashed_string_like<HashedStr32, HashDigest32, char>);

/// @brief Zero-terminated hashed non-owning string view
/// @warning Make sure that actual string data outlives `BasicHashedCStrView` that refers to it
/// @note Neither `BasicHashedCStrView` nor `BasicStringId` is polymorphic. Object slicing is the
/// intended behavior
template<class Digest, class Char>
class BasicHashedCStrView: public BasicStringId<Digest, Char> {
public:
	using StringIdType = BasicStringId<Digest, Char>;
	using ValueType = const Char*;
	using IsOwning = FalseC;

	BasicHashedCStrView() = delete;

	explicit constexpr
	BasicHashedCStrView(AdoptInit, Digest hash, const Char* str) noexcept:
		StringIdType{adopt, hash}, _str{str}
	{
		FR_ASSERT_AUDIT_MSG(hash == StringIdType::calc_hash(str, str_length(str)),
			"Incorrect hash value");
	}

	template<size_t N>
	explicit(false) constexpr
	BasicHashedCStrView(const Char (&str)[N]) noexcept:
		StringIdType{str}, _str{str}
	{
		// assert in the base constructor
	}

	/// @warning Interval [str, str + length] cannot contain any null terminators
	explicit constexpr
	BasicHashedCStrView(const Char* str, size_t length) noexcept:
		StringIdType{str, length}, _str{str}
	{ }

	/// @warning `str` must be null-terminated
	explicit constexpr
	BasicHashedCStrView(const Char* str) noexcept:
		StringIdType{str, str_length(str)}, _str{str}
	{ }

	friend constexpr
	auto operator==(BasicHashedCStrView lhs, BasicHashedCStrView rhs) noexcept -> bool {
		return lhs._hash == rhs._hash;
	}

	friend consteval
	auto fr_describe(const BasicHashedCStrView&) noexcept {
		return class_desc<
			Attributes<HashableMode::OptIn>,
			Bases<StringIdType>,
			Field<&BasicHashedCStrView::_str>
		>;
	}

	auto str() const noexcept -> const Char* { return _str; }

private:
	const Char* _str;
};

using HashedCStrView32 = BasicHashedCStrView<HashDigest32, char>;
using HashedCStrView64 = BasicHashedCStrView<HashDigest64, char>;
using HashedCStrView = BasicHashedCStrView<DefaultStrDigest, char>;

static_assert(c_hashed_string_like<HashedCStrView32, HashDigest32, char>);

/// @brief Hashed non-owning string view
/// @warning Make sure that actual string data outlives `BasicHashedCStrView` that refers to it
/// @note Neither `BasicHashedStrView` nor `BasicStringId` is polymorphic. Object slicing is the
/// intended behavior
template<class Digest, class Char>
class BasicHashedStrView: public BasicStringId<Digest, Char> {
public:
	using StringIdType = BasicStringId<Digest, Char>;
	using StringViewType = std::basic_string_view<Char>;
	using ValueType = StringViewType;
	using IsOwning = FalseC;

	BasicHashedStrView() = delete;

	explicit constexpr
	BasicHashedStrView(AdoptInit, Digest hash, StringViewType str) noexcept:
		StringIdType{adopt, hash}, _str{str}
	{
		FR_ASSERT_AUDIT_MSG(hash == StringIdType::calc_hash(str.data(), str.size()),
			"Incorrect hash value");
	}

	explicit(false) constexpr
	BasicHashedStrView(StringViewType str) noexcept:
		StringIdType{str}, _str(str)
	{
		// assert in the base constructor
	}

	template<size_t N>
	explicit(false) constexpr
	BasicHashedStrView(const Char (&str)[N]) noexcept:
		StringIdType{str}, _str(str, N - 1uz)
	{
		// assert in the base constructor
	}

	/// @warning Interval [str, str + length] cannot contain any null terminators
	explicit constexpr
	BasicHashedStrView(const Char* str, size_t length) noexcept:
		StringIdType{str, length}, _str(str, length)
	{ }

	friend constexpr
	auto operator==(BasicHashedStrView lhs, BasicHashedStrView rhs) noexcept -> bool {
		FR_ASSERT_AUDIT(lhs._hash == rhs._hash && std::strcmp(lhs._str, rhs._str) == 0);
		return lhs._hash == rhs._hash;
	}

	friend consteval
	auto fr_describe(const BasicHashedStrView&) noexcept {
		return class_desc<
			Attributes<HashableMode::OptIn>,
			Bases<StringIdType>,
			Field<&BasicHashedStrView::_str>
		>;
	}

	auto str() const noexcept -> StringViewType { return _str; }

private:
	StringViewType _str;
};

using HashedStrView32 = BasicHashedStrView<HashDigest32, char>;
using HashedStrView64 = BasicHashedStrView<HashDigest64, char>;
using HashedStrView = BasicHashedStrView<DefaultStrDigest, char>;

static_assert(c_hashed_string_like<HashedStrView32, HashDigest32, char>);

template<class Digest, class Char>
inline
auto operator==(
	BasicStringId<Digest, Char> lhs,
	const BasicHashedStr<Digest, Char>& rhs
) noexcept -> bool {
	return lhs.hash() == rhs.hash();
}

// TODO: deduction guides
// TODO: user-defined literals
// TODO: comparison operatos: StringId == HashedString, ...
// TODO: conversion operatos to hash and string types (?)

} // namespace fr
#endif // include guard
