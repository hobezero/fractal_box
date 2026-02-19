#ifndef FRACTAL_BOX_CORE_CONTAINERS_SPARSE_SET_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_SPARSE_SET_HPP

#include <concepts>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/bit_manip.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

namespace detail {

template<class T>
struct RawTypeDetector;

template<c_arithmetic T>
struct RawTypeDetector<T> {
	using Type = T;
};

template<c_enum T>
struct RawTypeDetector<T> {
	using Type = std::underlying_type_t<T>;
};

template<class T>
requires requires { typename T::RawType; }
struct RawTypeDetector<T> {
	using Type = typename T::RawType;
};

} // namespace detail

template<class T>
using RawUnderlying =  typename detail::RawTypeDetector<T>::Type;

template<int Bits>
inline constexpr auto default_sparse_version_bit_width = detail::MpIllegal{};

template<>
inline constexpr auto default_sparse_version_bit_width<32> = int{12};

template<>
inline constexpr auto default_sparse_version_bit_width<64> = int{32};

/// @brief Helper type for bitmask-based sparse key traits.
/// @details
///   - Layout: {version[VersionBitWidth] || location[bit_witdh<Key> - VersionBitWidth]}
///     Null key         : {1111||1111}
///     Invalidated keys : {xxxx||1111}
///     Min valid key    : {0000||0000}
///     Max valid key    : {1111||1110}
///   - Verisonless layout: {location[bit_width<Key>]}
///     Null key         : {11111111}
///     Invalidated keys : none
///     Min valid key    : {00000000}
///     Max valid key    : {11111110}
/// @todo
///   TODO: Max invalidated key {1111||1111} has the same value as the null key. Is that a problem?
///   TODO: Check if it's technically ill-formed to use enums with non-fixed underlying type
template<
	class Key,
	int VersionBitWidth = default_sparse_version_bit_width<bit_width<RawUnderlying<Key>>>
>
requires std::unsigned_integral<RawUnderlying<Key>>
	&& c_nothrow_explicitly_convertible_to<Key, RawUnderlying<Key>>
	&& c_nothrow_explicitly_convertible_to<RawUnderlying<Key>, Key>
	&& (0 <= VersionBitWidth && VersionBitWidth < bit_width<RawUnderlying<Key>>)
struct BitmaskSparseKeyTraits {
	using KeyType = Key;
	/// @note Always unsigned due to `Key` requirements
	using RawType = RawUnderlying<Key>;

	static constexpr int version_bit_width = VersionBitWidth;
	static constexpr int location_bit_width = bit_width<RawType> - VersionBitWidth;

private:
	static FR_FORCE_INLINE constexpr
	auto as_raw(KeyType key) noexcept -> RawType {
		// TODO: Add customization point for user-defined clases
		return static_cast<RawType>(key);
	}

	static FR_FORCE_INLINE constexpr
	auto as_key(RawType raw) noexcept -> KeyType {
		return static_cast<KeyType>(raw);
	}

	static constexpr auto version_mask = bitmask_high<RawType, version_bit_width>;
	static constexpr auto loc_mask = bitmask_low<RawType, location_bit_width>;

public:
	static constexpr bool has_version = VersionBitWidth > 0;

	static constexpr auto max_version = has_version
		? bitmask_low<RawType, version_bit_width>
		: RawType{0};
	static constexpr auto max_location = RawType{bitmask_low<RawType, location_bit_width> - 1u};

	static constexpr auto null = as_key(~RawType{0});

	static FR_FORCE_INLINE constexpr
	auto is_valid(KeyType key) noexcept -> bool {
		if constexpr (has_version)
			return (as_raw(key) & loc_mask) != loc_mask;
		else
			return key != null;
	}

	static FR_FORCE_INLINE constexpr
	auto location(KeyType key) noexcept -> RawType {
		FR_ASSERT_AUDIT(is_valid(key));
		return as_raw(key) & loc_mask;
	}

	static FR_FORCE_INLINE constexpr
	auto version(KeyType key) noexcept -> RawType {
		if constexpr (has_version) {
			return as_raw(key) >> location_bit_width;
		}
		else {
			return max_version;
		}
	}

	static FR_FORCE_INLINE constexpr
	auto make(RawType loc, RawType version) noexcept -> KeyType {
		FR_ASSERT_AUDIT(version <= max_version);
		FR_ASSERT_AUDIT(loc <= max_location);
		if constexpr (has_version)
			return as_key((version << location_bit_width) | loc);
		else
			return as_key(loc);
	}

	static FR_FORCE_INLINE constexpr
	auto make_new(RawType loc) noexcept -> KeyType { return as_key(loc); }

	static constexpr
	auto make_next(KeyType key) noexcept -> KeyType {
		if constexpr (has_version) {
			const auto raw = as_raw(key);
			// Set all lower bits to 1 so that addition will only affect the version part. Clear
			// them afterwards
			const auto high = static_cast<RawType>((raw | loc_mask) + 1u) & version_mask;
			return as_key(high | (raw & loc_mask));

			// TODO: Investigate alternative shift-based implementation:
			// ```
			// const auto mask = bitmask_low<RawType, version_bit_width>;
			// const auto high = (static_cast<RawType>((raw >> version_post_bit_offset) + 1u)
			//   & mask) << version_post_bit_offset;
			// return as_key(high | (raw & loc_mask));
			// ```
		}
		else {
			return key;
		}
	}

	static constexpr
	auto make_invalidated(KeyType key) noexcept -> KeyType
	requires has_version {
		return as_key(as_raw(key) | loc_mask);
	}
};

/// @brief Sparse set data structure
/// @details
/// Features:
///  - O(1) insertion/removal
///  - O(1) search
///  - O(N) iteration
///  - contiguous storage for keys
///  - optional versioning
///  - ...
/// @todo TODO: Exception safety
template<class Key, class Traits = BitmaskSparseKeyTraits<Key>>
requires (sizeof(typename Traits::RawType) <= sizeof(size_t))
class SparseIndexSet {
	using DenseContainer = std::vector<Key>;
	using SparseContainer = std::vector<Key>;

public:
	using KeyType = Key;
	using ValueType = Key;
	using SizeType = size_t;
	using TraitsType = Traits;
	using RawType = typename Traits::RawType;
	using Iterator = typename DenseContainer::iterator;
	using ConstIterator = typename DenseContainer::const_iterator;

	using key_type = Key;
	using value_type = Key;
	using traits_type = Traits;

	using size_type = SizeType; // Use Id here?
	using difference_type = std::ptrdiff_t;
	using reference = ValueType&;
	using const_reference = const ValueType&;
	using pointer = ValueType*;
	using const_pointer = const ValueType*;

	/// @todo TODO: Consider a custom iterator storing begin + index: for stability? EnTT does this
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	constexpr
	void swap(SparseIndexSet& other) noexcept {
		using std::swap;

		swap(_dense_keys, other._dense_keys);
		swap(_sparse_keys, other._sparse_keys);
		swap(_alive_count, other._alive_count);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(SparseIndexSet& a, SparseIndexSet& b) noexcept {
		a.swap(b);
	}

	constexpr
	auto create() -> Key {
		Key new_key;
		if (_alive_count == _dense_keys.size()) {
			// There are no invalidated slots available. Add a new slot at the end of each vector
			FR_ASSERT(_dense_keys.size() == _sparse_keys.size());
			FR_PANIC_CHECK(_dense_keys.size() <= size_t{Traits::max_location});

			new_key = Traits::make_new(static_cast<RawType>(_dense_keys.size()));
			_dense_keys.push_back(new_key);
			_sparse_keys.push_back(new_key);
		}
		else {
			// Recycle an invalidated slot
			const auto dense_idx = _alive_count.value();
			const auto revived = _dense_keys[dense_idx];
			new_key = Traits::make_next(revived);
			_dense_keys[dense_idx] = new_key;
			_sparse_keys[Traits::location(revived)] = Traits::make(static_cast<RawType>(dense_idx),
				Traits::version(new_key));
		}

		++_alive_count.value();
		return new_key;
	}

	constexpr
	auto try_erase(Key key) noexcept -> bool {
		// TODO: Optimize
		if (!contains(key))
			return false;
		erase(key);
		return true;
	}

	/// @pre `Traits::is_alive(key) && Traits::contains(key)`
	constexpr
	void erase(Key key) noexcept {
		FR_ASSERT(contains(key));

		const auto sparse_idx = size_t{Traits::location(key)};
		const auto old_sparse_key = _sparse_keys[sparse_idx];
		const auto old_loc = Traits::location(old_sparse_key);
		const auto new_loc = static_cast<RawType>(_alive_count.value() - 1zu);
		const auto plug_key = _dense_keys[new_loc];

		if (old_loc != new_loc) {
			// Replace the removed dense key with the last key
			// Move the removed dense key to the end
			std::swap(_dense_keys[old_loc], _dense_keys[new_loc]);

			_sparse_keys[Traits::location(plug_key)] = Traits::make(old_loc,
				Traits::version(plug_key));
		}
		if constexpr (Traits::has_version)
			_sparse_keys[sparse_idx] = Traits::make_invalidated(old_sparse_key);
		--_alive_count.value();
	}

	/// @pre `Traits::is_valid(key)`
	constexpr
	auto contains(Key key) const noexcept -> bool {
		FR_ASSERT(Traits::is_valid(key));

		const auto sparse_idx = size_t{Traits::location(key)};
		if (sparse_idx >= _sparse_keys.size())
			return false;
		const auto sparse_key = _sparse_keys[sparse_idx];
		if constexpr (Traits::has_version) {
			return Traits::is_valid(sparse_key)
				&& Traits::version(sparse_key) == Traits::version(key);
		}
		else {
			return _dense_keys[Traits::location(sparse_key)] == key;
		}
	}

	constexpr
	auto try_get_location(Key key) const noexcept -> RawType {
		if (!contains(key))
			return npos_for<RawType>;
		return Traits::location(_sparse_keys[Traits::location(key)]);
	}

	/// @pre `Traits::is_valid(key) && this->contains(key)`
	constexpr
	auto get_location_unchecked(Key key) const noexcept -> RawType {
		FR_ASSERT(contains(key));
		return Traits::location(_sparse_keys[Traits::location(key)]);
	}

	/// @post `this->empty()`
	constexpr
	void clear() noexcept {
		_dense_keys.clear();
		_sparse_keys.clear();
		_alive_count = 0zu;
	}

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> SizeType { return _alive_count.value(); }

	[[nodiscard]] FR_FORCE_INLINE
	auto empty() const noexcept -> bool { return _alive_count == 0uz; }

	FR_FORCE_INLINE constexpr
	auto is_empty() const noexcept -> bool { return _alive_count == 0uz; }

	FR_FORCE_INLINE constexpr
	auto keys() const noexcept -> std::span<const Key> {
		return {_dense_keys.begin(), _alive_count.value()};
	}

	FR_FORCE_INLINE constexpr
	auto begin() noexcept -> iterator { return _dense_keys.begin(); }

	FR_FORCE_INLINE constexpr
	auto begin() const noexcept -> const_iterator { return _dense_keys.begin(); }

	FR_FORCE_INLINE constexpr
	auto cbegin() const noexcept -> const_iterator { return _dense_keys.cbegin(); }

	FR_FORCE_INLINE constexpr
	auto end() noexcept -> iterator { return _dense_keys.end(); }

	FR_FORCE_INLINE constexpr
	auto end() const noexcept -> const_iterator { return _dense_keys.end(); }

	FR_FORCE_INLINE constexpr
	auto cend() const noexcept -> const_iterator { return _dense_keys.cend(); }

private:
	DenseContainer _dense_keys;
	SparseContainer _sparse_keys;
	WithDefault<size_t, 0> _alive_count;
};

template<class Key>
using SparseIndexSetVersionless = SparseIndexSet<Key, BitmaskSparseKeyTraits<Key, 0>>;

/// @brief A sparse set data structure that also stores a value for each key.
/// @details Similar to usual associative containers except keys are not provided by the user but
/// generated by the container itself.
/// Features:
///  - O(1) insertion/removal
///  - O(1) search
///  - O(N) iteration
///  - contiguous storage for keys AND values
///  - optional versioning
///  - ...
/// @todo TODO: Implement iterators over (key, value) pairs. Consider a custom iterator storing
/// begin + index: for stability? EnTT does this
template<class Key, class Value, class Traits = BitmaskSparseKeyTraits<Key>>
requires (sizeof(typename Traits::RawType) <= sizeof(size_t)) && c_nothrow_movable<Value>
class SparseMap {
	using DenseKeyContainer = std::vector<Key>;
	using DenseValueContainer = std::vector<Value>;
	using SparseKeyContainer = std::vector<Key>;

public:
	using KeyType = Key;
	using ValueType = Value;
	using TraitsType = Traits;
	using RawType = typename Traits::RawType;

	using key_type = Key;
	using value_type = Value;

	using size_type = size_t; // Use Id here?
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	constexpr
	void swap(SparseMap& other) noexcept {
		using std::swap;

		swap(_dense_values, other._dense_values);
		swap(_dense_keys, other._dense_keys);
		swap(_sparse_keys, other._sparse_keys);
		swap(_alive_count, other._alive_count);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(SparseMap& a, SparseMap& b) noexcept {
		a.swap(b);
	}

	template<class... Args>
	constexpr
	auto emplace(Args&&... args) -> Key {
		Key new_key;
		if (_alive_count == _dense_keys.size()) {
			// There are no invalidated slots available. Add a new slot at the end of each vector
			FR_ASSERT(_dense_keys.size() == _sparse_keys.size());
			FR_PANIC_CHECK(_dense_keys.size() <= size_t{Traits::max_location});

			_dense_values.emplace_back(std::forward<Args>(args)...);
			new_key = Traits::make_new(static_cast<RawType>(_dense_keys.size()));
			_dense_keys.push_back(new_key);
			_sparse_keys.push_back(new_key);
		}
		else {
			// Recycle an invalidated slot
			const auto dense_idx = _alive_count.value();
			const auto revived = _dense_keys[dense_idx];
			new_key = Traits::make_next(revived);
			// TODO: Can we save one move by replacing this line with `destroy_at` + `construct_at`?
			// Preserving exception guarantee (whatever that is) might be an issue
			_dense_values[dense_idx] = ValueType(std::forward<Args>(args)...);
			_dense_keys[dense_idx] = new_key;
			_sparse_keys[Traits::location(revived)] = Traits::make(static_cast<RawType>(dense_idx),
				Traits::version(new_key));
		}

		++_alive_count.value();
		return new_key;
	}

	FR_FORCE_INLINE constexpr
	auto insert(const ValueType& value) -> Key { return emplace(value); }

	FR_FORCE_INLINE constexpr
	auto insert(ValueType&& value) -> Key { return emplace(std::move(value)); }

	constexpr
	auto try_erase(Key key) -> bool {
		// TODO: Optimize
		if (!contains(key))
			return false;
		erase(key);
		return true;
	}

	/// @pre `Traits::is_valid(key) && Traits::contains(key)`
	constexpr
	void erase(Key key) noexcept {
		FR_ASSERT(contains(key));

		const auto sparse_idx = Traits::location(key);
		const auto old_sparse_key = _sparse_keys[sparse_idx];
		const auto old_loc = Traits::location(old_sparse_key);
		const auto new_loc = static_cast<RawType>(_alive_count.value() - 1zu);

		if (new_loc != old_loc) {
			const auto plug_key = _dense_keys[new_loc];
			// Replace the removed dense value with the last value
			// Move the removed dense value to the end
			std::swap(_dense_keys[old_loc], _dense_keys[new_loc]);
			_dense_values[old_loc] = std::move(_dense_values[new_loc]);
			_sparse_keys[Traits::location(plug_key)] = Traits::make(old_loc,
				Traits::version(plug_key));
		}
		if constexpr (Traits::has_version)
			_sparse_keys[sparse_idx] = Traits::make_invalidated(old_sparse_key);
		--_alive_count.value();
	}

	/// @pre `Traits::is_valid(key)`
	constexpr
	auto contains(Key key) const noexcept -> bool {
		FR_ASSERT(Traits::is_valid(key));

		const auto sparse_idx = size_t{Traits::location(key)};
		if (sparse_idx >= _sparse_keys.size())
			return false;
		const auto sparse_key = _sparse_keys[sparse_idx];
		if constexpr (Traits::has_version) {
			return Traits::is_valid(sparse_key)
				&& Traits::version(sparse_key) == Traits::version(key);
		}
		else {
			return _dense_keys[Traits::location(sparse_key)] == key;
		}
	}

	template<class Self>
	constexpr
	auto try_get_value(this Self& self, Key key) noexcept -> CopyConst<ValueType, Self>* {
		if (!self.contains(key))
			return nullptr;
		const auto loc = Traits::location(self._sparse_keys[Traits::location(key)]);
		return std::addressof(self._dense_values[loc]);
	}

	/// @pre `Traits::is_valid(key) && this->contains(key)`
	template<class Self>
	constexpr
	auto get_value_unchecked(this Self& self, Key key) noexcept -> CopyConst<ValueType, Self>& {
		FR_ASSERT(self.contains(key));
		const auto loc = Traits::location(self._sparse_keys[Traits::location(key)]);
		return self._dense_values[loc];
	}

	constexpr
	auto try_get_location(Key key) const noexcept -> RawType {
		if (!contains(key))
			return npos_for<RawType>;
		return Traits::location(_sparse_keys[Traits::location(key)]);
	}

	/// @pre `Traits::is_valid(key) && this->contains(key)`
	constexpr
	auto get_location_unchecked(Key key) const noexcept -> RawType {
		FR_ASSERT(contains(key));
		return Traits::location(_sparse_keys[Traits::location(key)]);
	}

	/// @post `this->empty()`
	constexpr
	void clear() noexcept {
		_dense_values.clear();
		_dense_keys.clear();
		_sparse_keys.clear();
		_alive_count = 0zu;
	}

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> size_type { return _alive_count.value(); }

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto empty() const noexcept -> bool { return _alive_count == 0zu; }

	FR_FORCE_INLINE constexpr
	auto is_empty() const noexcept -> bool { return _alive_count == 0zu; }

	FR_FORCE_INLINE constexpr
	auto keys() const noexcept -> std::span<const Key> {
		return {_dense_keys.begin(), _alive_count.value()};
	}

	FR_FORCE_INLINE constexpr
	auto values() const noexcept -> std::span<const ValueType> {
		return {_dense_values.begin(), _alive_count.value()};
	}

	FR_FORCE_INLINE constexpr
	auto values() noexcept -> std::span<ValueType> {
		return {_dense_values.begin(), _alive_count.value()};
	}

private:
	DenseValueContainer _dense_values;
	DenseKeyContainer _dense_keys;
	SparseKeyContainer _sparse_keys;
	WithDefault<size_t, 0zu> _alive_count;
};

template<class Key, class Value>
using SparseMapVersionless = SparseMap<Key, Value, BitmaskSparseKeyTraits<Key, 0>>;

} // namespace fr
#endif // include guard
