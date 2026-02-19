#ifndef FRACTAL_BOX_CORE_CONTAINERS_LINEAR_FLAT_SET_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_LINEAR_FLAT_SET_HPP

#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/iterator_utils.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @brief An associative container adapter that models an unordered set of unique objects
/// of type Key
/// @details
/// Search, insertion, and removal have average linear complexity.
/// Suitable for a small number of elements.
/// Const functions never throw unless KeyEqual throws. Other functions provide basic exception
/// guarantee unless specified otherwise
/// @todo
///   TODO: Look into C++23's `std::flat_set`
///   TODO: Support transparent KeyEqual
///   TODO: Concepts. Container should probably statisfy the requirements of SequenceContainer
///   TODO: Proper container concepts instead of range ones
template<
	class Key,
	class KeyEqual = EqualTo<Key>,
	class Container = std::vector<Key>
>
requires std::ranges::bidirectional_range<Container> && std::ranges::sized_range<Container>
class LinearFlatSet {
	static constexpr bool is_cmp_nothrow
		= noexcept(KeyEqual{}(std::declval<const Key&>(), std::declval<const Key&>()));

public:
	using ContainerType = Container;
	using SizeType = typename Container::size_type;
	using KeyType = Key;
	using ValueType= Key;
	using Iterator = typename Container::iterator;
	using ConstIterator = typename Container::const_iterator;
	using InsertResult = InsertIterResult<ConstIterator>;

	using container_type = ContainerType;
	using key_type = KeyType;
	using value_type = ValueType;
	using size_type = SizeType;
	using difference_type = typename Container::difference_type;
	using key_equal = KeyEqual;
	using allocator_type = typename Container::allocator_type;
	using reference = typename Container::reference;
	using const_reference = typename Container::const_reference;
	using pointer = typename std::allocator_traits<allocator_type>::pointer;
	using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	// Special functions
	// ^^^^^^^^^^^^^^^^^

	LinearFlatSet() = default;

	explicit constexpr
	LinearFlatSet(AdoptInit, const Container& c): _buff(c) { }

	explicit constexpr
	LinearFlatSet(AdoptInit, Container&& c)
	noexcept(std::is_nothrow_move_constructible_v<Container>):
		_buff(std::move(c))
	{ }

	constexpr
	void swap(LinearFlatSet& other)
	noexcept(std::is_nothrow_swappable_v<Container>) {
		using std::swap;
		swap(_buff, other._buff);
	}

	friend constexpr
	void swap(LinearFlatSet& lhs, LinearFlatSet& rhs)
	noexcept(noexcept(lhs.swap(rhs))) {
		using std::swap;
		swap(lhs._buff, rhs._buff);
	}

	// Iterators
	// ^^^^^^^^^

	constexpr
	auto begin() const noexcept -> ConstIterator { return _buff.cbegin(); }

	constexpr
	auto cbegin() const noexcept -> ConstIterator { return _buff.cbegin(); }

	constexpr
	auto end() const noexcept -> ConstIterator { return _buff.cend(); }

	constexpr
	auto cend() const noexcept -> ConstIterator { return _buff.cend(); }

	// Capacity
	// ^^^^^^^^

	[[nodiscard]] constexpr
	auto empty() const noexcept -> bool { return _buff.empty(); }

	constexpr
	auto size() const noexcept -> SizeType { return _buff.size(); }

	constexpr
	auto capacity() const noexcept -> SizeType
	requires requires(Container b) { { b.capacity() } -> std::same_as<SizeType>; } {
		return _buff.capacity();
	}

	// Modifiers
	// ^^^^^^^^^

	constexpr
	void clear() noexcept(noexcept(_buff.clear())) {
		_buff.clear();
	}

	constexpr
	void reserve(ConstIterator size) noexcept(noexcept(_buff.reserve(size)))
	requires requires(Container b) { b.reserve(size); } {
		_buff.reserve(size);
	}

	constexpr
	auto insert(const Key& value) -> InsertResult {
		if (auto it_old = find_impl(_buff, value); it_old != _buff.cend()) {
			return {std::move(it_old), false};
		}
		_buff.push_back(value);
		return {std::ranges::prev(_buff.cend()), true};
	}

	constexpr
	auto insert(Key&& value) -> InsertResult {
		if (auto it_old = find_impl(_buff, value); it_old != _buff.cend()) {
			return {std::move(it_old), false};
		}
		_buff.push_back(std::move(value));
		return {std::ranges::prev(_buff.cend()), true};
	}

	/// @pre `!this->contains(value)`
	constexpr
	auto insert_unchecked(const Key& value) -> Iterator {
		_buff.push_back(value);
		return std::ranges::prev(_buff.cend());
	}

	/// @pre `!this->contains(value)`
	constexpr
	auto insert_unchecked(Key&& value) -> Iterator {
		_buff.push_back(std::move(value));
		return std::ranges::prev(_buff.cend());
	}

	/// @return A pair consisting of an iterator to the inserted element, or
	/// the already-existing element if no insertion happened, and a bool denoting whether
	/// the insertion took place (true if insertion happened, false if it did not)
	template<class... Args>
	constexpr
	auto emplace(Args&&... args) -> InsertResult {
		// NOTE: emplace_back may invalidate
		const auto& value = _buff.emplace_back(std::forward<Args>(args)...);
		auto it_new = std::ranges::prev(_buff.cend());
		if (auto it_old = find_impl(_buff.cbegin(), it_new, value); it_old != it_new) {
			// TODO: Can `pop_back()` invalidate `it_old` in some containers? What if it throws?
			_buff.pop_back();
			return {std::move(it_old), false};
		}
		return {std::move(it_new), true};
	}

	/// @pre `!contains(Key(std::forward<Args>(args)...))`
	template<class... Args>
	constexpr
	auto emplace_unchecked(Args&&... args) -> Iterator {
		_buff.emplace_back(std::forward<Args>(args)...);
		const auto it_last = std::ranges::prev(_buff.cend());
		FR_ASSERT_AUDIT(find_impl(_buff.cbegin(), it_last) == it_last);
		return it_last;
	}

	constexpr
	auto erase(ConstIterator pos) -> Iterator { return _buff.erase(pos); }

	constexpr
	auto erase(Iterator pos) -> Iterator { return _buff.erase(pos); }

	/// @return Number of elements removed (0 or 1)
	constexpr
	auto erase(const Key& key) -> SizeType {
		if (const auto it = find(key); it != _buff.cend()) {
			_buff.erase(it);
			return 1;
		}
		return 0;
	}

	constexpr
	auto release_buffer() noexcept -> Container {
		auto buff = std::move(_buff);
		_buff.clear(); // Make sure that the buffer isn't left in an unspecified moved-from state
		return buff;
	}

	// Lookup
	// ^^^^^^

	constexpr
	auto find(const Key& key) noexcept(is_cmp_nothrow) -> ConstIterator {
		return find_impl(_buff, key);
	}

	constexpr
	auto find(const Key& key) const noexcept(is_cmp_nothrow) -> ConstIterator {
		return find_impl(_buff, key);
	}

	constexpr
	auto contains(const Key& key) const -> bool {
		auto cmp = key_equal{};
		for (const auto& e : _buff) {
			if (cmp(e, key)) {
				return true;
			}
		}
		return false;
	}

	// Element access
	// ^^^^^^^^^^^^^^

	constexpr
	auto data() const noexcept -> const ValueType*
	requires std::ranges::contiguous_range<Container> {
		return std::ranges::data(_buff);
	}

	// Friends
	// ^^^^^^^

	/// @warning O(N^2) time complexity
	friend constexpr
	auto operator==(const LinearFlatSet& lhs, const LinearFlatSet& rhs) noexcept -> bool {
		if (lhs.size() != rhs.size())
			return false;

		for (const auto& e : lhs._buff) {
			// one-to-one relationship (?)
			if (!rhs.contains(e))
				return false;
		}
		return true;
	}

private:
	template<class Iter, class Sentinel>
	static FR_FORCE_INLINE constexpr
	auto find_impl(Iter begin, Sentinel end, const Key& key) noexcept(is_cmp_nothrow) -> Iter {
		auto cmp = key_equal{};
		for (; begin != end; ++begin) {
			if (cmp(*begin, key)) {
				return begin;
			}
		}
		return end;
	}

	template<class R>
	static FR_FORCE_INLINE constexpr
	auto find_impl(R&& r, const Key& key) noexcept(is_cmp_nothrow) {
		return find_impl(r.cbegin(), r.cend(), key);
	}

private:
	Container _buff;
};

} // namespace fr
#endif // include guard
