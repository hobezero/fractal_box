#ifndef FRACTAL_BOX_CORE_HASHING_HASHER_VISITOR_BASE_HPP
#define FRACTAL_BOX_CORE_HASHING_HASHER_VISITOR_BASE_HPP

#include <iterator>
#include <tuple>

#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/hashing/hash_digest.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/range_concepts.hpp"

namespace fr {

struct HasherVisitorBase {
	// Digest
	// ^^^^^^

	template<c_hash_digest T>
	struct Digest {
		T value;
	};

	template<c_hash_digest T>
	static FR_FORCE_INLINE constexpr
	auto digest(T digest) noexcept -> Digest<T> {
		return Digest<T>{digest};
	}

	// Bytes
	// ^^^^^

	template<c_byte_like B, std::integral SizeType>
	struct Bytes {
		const B* data;
		SizeType size;
	};

	template<c_byte_like B, std::integral SizeType>
	static FR_FORCE_INLINE constexpr
	auto bytes(const B* data, SizeType size) noexcept -> Bytes<B, SizeType> {
		return {data, size};
	}

	template<c_dynamic_extent_span_like Span>
	requires (c_byte_like<typename Span::value_type>)
	static FR_FORCE_INLINE constexpr
	auto bytes(Span span) noexcept -> Bytes<typename Span::value_type, size_t> {
		return {span.data(), span.size_bytes()};
	}

	// FixedBytes
	// ^^^^^^^^^^

	template<c_byte_like B, auto Size>
	struct FixedBytes {
		static constexpr auto size = Size;

	public:
		const B* data;
	};

	template<auto Size, c_byte_like B>
	static FR_FORCE_INLINE constexpr
	auto fixed_bytes(const B* data) noexcept -> FixedBytes<B, Size> {
		return {data};
	}

	template<c_const_extent_span_like Span>
	requires (c_byte_like<typename Span::value_type>)
	static FR_FORCE_INLINE constexpr
	auto fixed_bytes(Span span) noexcept -> FixedBytes<typename Span::value_type, Span::extent> {
		return {span.data()};
	}

	template<c_const_extent_span_like Span>
	requires (c_byte_like<typename Span::value_type>)
	static FR_FORCE_INLINE constexpr
	auto bytes(Span span) noexcept -> FixedBytes<typename Span::value_type, Span::extent> {
		return {span.data()};
	}

	// String
	// ^^^^^^

	template<c_character Char, std::integral SizeType>
	struct String {
		const Char* data;
		SizeType size;
	};

	template<c_character Char, std::integral SizeType>
	static FR_FORCE_INLINE constexpr
	auto string(const Char* data, SizeType size) -> String<Char, SizeType> {
		return {data, size};
	}

	template<c_string_view_like SView>
	static FR_FORCE_INLINE constexpr
	auto string(SView str) -> String<typename SView::value_type, typename SView::size_type> {
		return {str.data(), str.size()};
	}

	template<c_string_like SView>
	static FR_FORCE_INLINE constexpr
	auto string(SView str) -> String<typename SView::value_type, typename SView::size_type> {
		return {str.data(), str.size()};
	}

	// FixedString
	// ^^^^^^^^^^^

	template<c_character Char, auto Size>
	requires std::integral<decltype(Size)>
	struct FixedString {
		static constexpr auto size = Size;
		const Char* data;
	};

	template<auto Size, c_character C>
	requires std::integral<decltype(Size)>
	static FR_FORCE_INLINE constexpr
	auto fixed_string(const C* data) noexcept -> FixedString<C, Size> {
		return {data};
	}

	template<c_const_extent_span_like Span>
	requires (c_character<typename Span::value_type>)
	static FR_FORCE_INLINE constexpr
	auto fixed_string(Span span) noexcept -> FixedString<typename Span::value_type, Span::extent> {
		return {span.data()};
	}

	// Ptr
	// ^^^

	/// @todo TODO: Ban recursive `Ptr`'s
	template<class T>
	class Ptr {
	public:
		using ValueType = T;

		explicit FR_FORCE_INLINE constexpr
		Ptr(const T* ptr) noexcept: _ptr{ptr} { }

		FR_FORCE_INLINE constexpr
		auto get() const noexcept -> const T* { return _ptr; }

	private:
		const T* _ptr;
	};

	template<class T>
	Ptr(T*) -> Ptr<T>;

	/// @note Not available at compile time
	template<class T>
	static FR_FORCE_INLINE
	auto ptr(const T* ptr) noexcept { return Ptr<T>{ptr}; }

	// Tuple
	// ^^^^^

	template<c_object... Ts>
	struct Tuple {
		using Args = MpList<Ts...>;
		static constexpr auto size = sizeof...(Ts);

		explicit FR_FORCE_INLINE constexpr
		Tuple(const Ts&... objs) noexcept: args{objs...} { }

	public:
		std::tuple<const Ts&...> args;
	};

	template<c_object... Ts>
	requires (sizeof...(Ts) > 0)
	static FR_FORCE_INLINE constexpr
	auto tuple(const Ts&... args) noexcept -> Tuple<Ts...> {
		return Tuple<Ts...>{args...};
	}

	// CommutativeTuple
	// ^^^^^^^^^^^^^^^^

	template<c_object... Ts>
	struct CommutativeTuple {
		using Args = MpList<Ts...>;
		static constexpr auto size = sizeof...(Ts);

		explicit FR_FORCE_INLINE constexpr
		CommutativeTuple(const Ts&... objs) noexcept: args{objs...} { }

	public:
		std::tuple<const Ts&...> args;
	};

	template<c_object... Ts>
	requires (sizeof...(Ts) > 1)
	static FR_FORCE_INLINE constexpr
	auto commutative_tuple(const Ts&... args) noexcept -> CommutativeTuple<Ts...> {
		return CommutativeTuple<Ts...>{args...};
	}

	// Range
	// ^^^^^

	template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel, size_t Size = npos>
	struct Range {
		using IteratorType = Iter;
		using SentinelType = Sentinel;
		static constexpr auto size = Size;

	public:
		Iter begin;
		Sentinel end;
	};

	template<class Iter, class Sentinel>
	Range(Iter, Sentinel) -> Range<Iter, Sentinel>;

	template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
	static FR_FORCE_INLINE constexpr
	auto range(Iter begin, Sentinel end) noexcept {
		return Range{begin, end};
	}

	template<std::ranges::input_range R>
	static FR_FORCE_INLINE constexpr
	auto range(R&& r) noexcept {
		using It = std::ranges::iterator_t<R>;
		using St = std::ranges::sentinel_t<R>;
		if constexpr (c_constexpr_sized_range<R>) {
			return Range<It, St, std::ranges::size(r)>{std::ranges::begin(r), std::ranges::end(r)};
		}
		else {
			return Range<It, St>{std::ranges::begin(r), std::ranges::end(r)};
		}
	}

	// CommutativeRange
	// ^^^^^^^^^^^^^^^^^

	template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel, size_t Size = npos>
	struct CommutativeRange {
		using IteratorType = Iter;
		using SentinelType = Sentinel;
		static constexpr auto size = Size;

	public:
		Iter begin;
		Sentinel end;
	};

	template<class Iter, class Sentinel>
	CommutativeRange(Iter, Sentinel) -> CommutativeRange<Iter, Sentinel>;

	template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
	static FR_FORCE_INLINE constexpr
	auto commutative_range(Iter begin, Sentinel end) noexcept -> CommutativeRange<Iter, Sentinel>{
		return {begin, end};
	}

	template<std::ranges::input_range R>
	static FR_FORCE_INLINE constexpr
	auto commutative_range(R&& r) noexcept {
		using It = std::ranges::iterator_t<R>;
		using St = std::ranges::sentinel_t<R>;
		if constexpr (c_constexpr_sized_range<R>) {
			return CommutativeRange<It, St, std::ranges::size(r)>{
				std::ranges::begin(r),
				std::ranges::end(r)
			};
		}
		else {
			return CommutativeRange<It, St>{std::ranges::begin(r), std::ranges::end(r)};
		}
	}

	// Optional
	// ^^^^^^^^

	template<c_object T>
	struct Optional {
		using ValueType = T;

	public:
		const T* value;
	};

	template<c_object T>
	static FR_FORCE_INLINE constexpr
	auto optional(const T* value) -> Optional<T> {
		return {value};
	}
};

// Digest
// ^^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_digest = false;

template<class D>
inline constexpr auto is_hvb_wrapper_digest<HasherVisitorBase::Digest<D>> = true;

// Bytes
// ^^^^^
template<class T>
inline constexpr auto is_hvb_wrapper_bytes = false;

template<class B, class SizeType>
inline constexpr auto is_hvb_wrapper_bytes<HasherVisitorBase::Bytes<B, SizeType>>
	= true;

// FixedBytes
// ^^^^^^^^^^
template<class T>
inline constexpr auto is_hvb_wrapper_fixed_bytes = false;

template<class B, auto Size>
inline constexpr auto is_hvb_wrapper_fixed_bytes<HasherVisitorBase::FixedBytes<B, Size>> = true;

// String
// ^^^^^^
template<class T>
inline constexpr auto is_hvb_wrapper_string = false;

template<class Char, class SizeType>
inline constexpr auto is_hvb_wrapper_string<HasherVisitorBase::String<Char, SizeType>> = true;

// FixedfString
// ^^^^^^^^^^^^
template<class T>
inline constexpr auto is_hvb_wrapper_fixed_string = false;

template<class Char, auto Size>
inline constexpr auto is_hvb_wrapper_fixed_string<HasherVisitorBase::FixedString<Char, Size>>
	= true;

// Ptr
// ^^^

template<class T>
inline constexpr auto is_hvb_wrapper_ptr = false;

template<class T>
inline constexpr auto is_hvb_wrapper_ptr<HasherVisitorBase::Ptr<T>> = true;

// Tuple
// ^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_tuple = false;

template<class... Ts>
inline constexpr auto is_hvb_wrapper_tuple<HasherVisitorBase::Tuple<Ts...>> = true;

// CommutativeTuple
// ^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_commutative_tuple = false;

template<class... Ts>
inline constexpr auto is_hvb_wrapper_commutative_tuple<HasherVisitorBase::CommutativeTuple<Ts...>>
	= true;

// Range
// ^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_range = false;

template<class Iter, class Sentinel, size_t Size>
inline constexpr auto is_hvb_wrapper_range<HasherVisitorBase::Range<Iter, Sentinel, Size>> = true;

// CommutativeRange
// ^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_commutative_range = false;

template<class Iter, class Sentinel, size_t Size>
inline constexpr auto is_hvb_wrapper_commutative_range<
	HasherVisitorBase::CommutativeRange<Iter, Sentinel, Size>> = true;

// Optional
// ^^^^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper_optional = false;

template<class T>
inline constexpr auto is_hvb_wrapper_optional<HasherVisitorBase::Optional<T>> = true;

// Any of the wrappers
// ^^^^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_hvb_wrapper
	= is_hvb_wrapper_digest<T>
	|| is_hvb_wrapper_bytes<T>
	|| is_hvb_wrapper_fixed_bytes<T>
	|| is_hvb_wrapper_string<T>
	|| is_hvb_wrapper_fixed_string<T>
	|| is_hvb_wrapper_ptr<T>
	|| is_hvb_wrapper_tuple<T>
	|| is_hvb_wrapper_commutative_tuple<T>
	|| is_hvb_wrapper_range<T>
	|| is_hvb_wrapper_commutative_range<T>
	|| is_hvb_wrapper_optional<T>
;

template<class T>
concept c_hvb_wrapper = is_hvb_wrapper<T>;

} // namespace fr
#endif // include guard
