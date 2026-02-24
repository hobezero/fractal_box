#ifndef FRACTAL_BOX_CORE_CONTAINERS_TYPE_OBJECT_MAP_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_TYPE_OBJECT_MAP_HPP

#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/iterator_utils.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/type_index.hpp"

namespace fr {

/// @brief Iterator for TypeObjectMap
template<c_type_index_domain Domain, c_object V, bool IsConst>
class TypeObjectMapIter {
	using TypeIdx = TypeIndex<Domain>;
	using Opts = AddConstIf<std::vector<std::optional<V>>, IsConst>;

public:
	using key_type = TypeIndex<Domain>; // // Not used by iterator_traits
	using mapped_type = V; // Not used by iterator_traits

	using iterator_concept = std::forward_iterator_tag;
	using iterator_category = std::forward_iterator_tag;
	using value_type = std::pair<const key_type, mapped_type>;
	using difference_type = typename Opts::difference_type;
	using reference = std::pair<const key_type, MpIf<IsConst, const mapped_type&, mapped_type&>>;
	using pointer = ArrowProxy<reference>;

	TypeObjectMapIter() = default;

	explicit FR_FORCE_INLINE constexpr
	TypeObjectMapIter(Opts& objects, size_t curr_idx) noexcept:
		_objects{std::addressof(objects)},
		_curr_idx{curr_idx}
	{ }

	explicit(false) constexpr
	operator TypeObjectMapIter<Domain, V, true>() noexcept
	requires (!IsConst) {
		return TypeObjectMapIter<Domain, V, true>{*_objects, _curr_idx};
	}

	FR_FORCE_INLINE constexpr
	auto operator*() const -> reference {
		FR_ASSERT_AUDIT(_objects);
		using DomainValue = typename Domain::ValueType;
		return {TypeIdx{adopt, static_cast<DomainValue>(_curr_idx)}, *(*_objects)[_curr_idx]};
	}

	FR_FORCE_INLINE
	auto operator->() const -> pointer {
		return pointer{operator*()};
	}

	auto operator++() noexcept -> TypeObjectMapIter& {
		auto end_idx = _objects->size();
		FR_ASSERT_AUDIT(_curr_idx != end_idx);
		++_curr_idx;
		while (_curr_idx != end_idx && !(*_objects)[_curr_idx].has_value())
			++_curr_idx;
		return *this;
	}

	auto operator++(int) noexcept -> TypeObjectMapIter {
		auto tmp = *this;
		++*this;
		return tmp;
	}

	constexpr
	auto operator==(TypeObjectMapIter rhs) const noexcept -> bool {
		FR_ASSERT_AUDIT(_objects == rhs._objects);
		return _curr_idx == rhs._curr_idx;
	}

private:
	/// @note We can't get away with storing just an iterator to the original container because
	/// `operator++()` needs to know when to stop
	Opts* _objects = nullptr;
	size_t _curr_idx = 0zu;
};

/// @todo TODO: Maybe follow `std::map` API design?
template<c_type_index_domain Domain, c_object V>
requires std::movable<V>
class TypeObjectMap {
	using OptV = std::optional<V>;

public:
	using DomainType = Domain;
	using TypeIndexType = TypeIndex<Domain>;

	using KeyType = TypeIndexType;
	using MappedType = V;
	using ValueType = std::pair<const KeyType, MappedType>;
	using SizeType = size_t;
	using ConstIterator = TypeObjectMapIter<Domain, V, true>;
	using Iterator = TypeObjectMapIter<Domain, V, false>;

	using key_type = TypeIndexType;
	using mapped_type = MappedType;
	using value_type = ValueType;
	using size_type = SizeType;
	using difference_type = ptrdiff_t;
	using reference = std::pair<const key_type, mapped_type&>;
	using const_reference = std::pair<const key_type, const mapped_type&>;
	using pointer = ArrowProxy<reference>;
	using const_pointer = ArrowProxy<const reference>;
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	constexpr
	void swap(TypeObjectMap& other) noexcept {
		using std::swap;
		swap(_objects, other._objects);
		swap(_count, other._count);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(TypeObjectMap& lhs, TypeObjectMap& rhs) noexcept {
		lhs.swap(rhs);
	}

	template<class T, class... Args>
	requires std::constructible_from<V, Args...>
	FR_FORCE_INLINE
	auto try_emplace(Args&&... args) -> InsertIterResult<iterator> {
		return try_emplace_at(type_index<T, Domain>, std::forward<Args>(args)...);
	}

	template<class... Args>
	requires std::constructible_from<V, Args...>
	constexpr
	auto try_emplace_at(TypeIndexType type_idx, Args&&... args) -> InsertIterResult<iterator> {
		const auto idx = SizeType{type_idx.value()};

		if (idx < _objects.size()) {
			if (_objects[idx].has_value())
				return {iterator{_objects, idx}, false};
			_objects[idx].emplace(std::forward<Args>(args)...);
		}
		else if (idx == _objects.size()) {
			_objects.emplace_back(std::in_place, std::forward<Args>(args)...);
		}
		else {
			_objects.resize(static_cast<size_t>(idx + SizeType{1}));
			_objects.back().emplace(std::forward<Args>(args)...);
		}

		if (_count == SizeType{0} || idx < _first_idx)
			_first_idx = idx;
		++_count.value();
		return {Iterator{_objects, idx}, true};
	}

	template<class Self>
	constexpr
	auto try_get_at(this Self& self, TypeIndexType type_idx) -> CopyConst<V, Self>* {
		const auto idx = SizeType{type_idx.value()};
		if (idx >= self._objects.size() || !self._objects[idx].has_value())
			return nullptr;
		return self._objects[idx].operator->();
	}

	/// @todo TODO: Write unit tests
	template<class Self>
	constexpr
	auto get_at(this Self&& self, TypeIndexType type_idx) -> FwdLike<V, Self> {
		const auto idx = SizeType{type_idx.value()};
		FR_PANIC_CHECK(idx < self._objects.size() && self._objects[idx].has_value());
		return std::forward_like<Self>(*self._objects[idx]);
	}

	template<class Self>
	constexpr
	auto get_at_unchecked(this Self&& self, TypeIndexType type_idx) -> FwdLike<V, Self> {
		const auto idx = SizeType{type_idx.value()};
		FR_ASSERT(idx < self._objects.size() && self._objects[idx].has_value());
		return std::forward_like<Self>(*self._objects[idx]);
	}

	template<class T, class Self>
	FR_FORCE_INLINE
	auto try_get(this Self& self) -> CopyConst<mapped_type, Self>* {
		return self.try_get_at(type_index<T, Domain>);
	}

	template<class T, class Self>
	FR_FORCE_INLINE
	auto get(this Self&& self) -> CopyCvRef<mapped_type, Self>&& {
		return std::forward<Self>(self).get_at(type_index<T, Domain>);
	}

	template<class T, class Self>
	FR_FORCE_INLINE
	auto get_unchecked(this Self&& self) -> CopyCvRef<mapped_type, Self>&& {
		return std::forward<Self>(self).get_at_unchecked(type_index<T, Domain>);
	}

	void clear() noexcept {
		_objects.clear();
		_count.reset();
		_first_idx.reset();
	}

	static FR_FORCE_INLINE constexpr
	auto max_size() noexcept -> SizeType {
		return std::numeric_limits<typename Domain::value_type>::max();
	}

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> SizeType { return _count.value(); }

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto empty() const noexcept -> bool { return _count == SizeType{0}; }

	FR_FORCE_INLINE constexpr
	auto is_empty() const noexcept -> bool { return _count == 0u; }

	FR_FORCE_INLINE constexpr
	auto cbegin() const noexcept -> ConstIterator {
		return ConstIterator{_objects, _first_idx.value()};
	}

	FR_FORCE_INLINE constexpr
	auto begin() const noexcept -> ConstIterator {
		return ConstIterator{_objects, _first_idx.value()};
	}

	FR_FORCE_INLINE constexpr
	auto begin() noexcept -> Iterator { return Iterator{_objects, _first_idx.value()}; }

	FR_FORCE_INLINE constexpr
	auto cend() const noexcept -> ConstIterator { return ConstIterator{_objects, _objects.size()}; }

	FR_FORCE_INLINE constexpr
	auto end() const noexcept -> ConstIterator { return ConstIterator{_objects, _objects.size()}; }

	FR_FORCE_INLINE constexpr
	auto end() noexcept -> Iterator { return Iterator{_objects, _objects.size()}; }

private:
	std::vector<OptV> _objects;
	WithDefaultValue<SizeType{0}> _count;
	/// @note Necessary to make `begin()` O(1), as required by the standard
	WithDefaultValue<SizeType{0}> _first_idx;
};

class AnyDeleter {
public:
	explicit constexpr
	AnyDeleter() noexcept: _fn{nullptr} { }

	template<class T>
	explicit constexpr
	AnyDeleter(InPlaceAsInit<T>) noexcept {
		_fn = +[](void* type_erased) { delete static_cast<T*>(type_erased); };
	}

	FR_FORCE_INLINE
	void operator()(void* p) const noexcept { _fn(p); }

private:
	using Fn = void (*)(void*);

	Fn _fn;
};

using AnyUnique = std::unique_ptr<void, AnyDeleter>;

template<class T, class... Args>
FR_FORCE_INLINE constexpr
auto make_any_unique(Args&&... args) -> AnyUnique {
	return AnyUnique{new T(std::forward<Args>(args)...), AnyDeleter{in_place_as<T>}};
}

template<c_type_index_domain Domain>
class TypeAnyObjectMap {
public:
	using DomainType = Domain;
	using SizeType = size_t;
	using TypeIndexType = TypeIndex<Domain>;
	template<class T>
	using InsertResult = InsertValueResult<T>;

	TypeAnyObjectMap() = default;

	TypeAnyObjectMap(const TypeAnyObjectMap&) = delete;
	auto operator=(const TypeAnyObjectMap&) -> TypeAnyObjectMap& = delete;

	TypeAnyObjectMap(TypeAnyObjectMap&&) = default;
	auto operator=(TypeAnyObjectMap&&) -> TypeAnyObjectMap& = default;

	constexpr
	~TypeAnyObjectMap() {
		for (auto i = _insertion_order.size(); i-- > 0uz; )
			_objects[_insertion_order[i]].reset();
	}

	template<class T, class... Args>
	requires std::constructible_from<std::decay_t<T>, Args...>
	auto try_emplace(Args&&... args) -> InsertResult<std::decay_t<T>> {
		const auto idx = size_t{type_index<T, Domain>.value()};
		using R = std::decay_t<T>;
		R* ptr;

		if (idx < _objects.size()) {
			auto& any = _objects[idx];
			if (any)
				return {*static_cast<R*>(any.get()), false};
			any = make_any_unique<R>(std::forward<Args>(args)...);
			ptr = static_cast<R*>(any.get());
		}
		else if (idx == _objects.size()) {
			auto& any = _objects.emplace_back(make_any_unique<R>(std::forward<Args>(args)...));
			ptr = static_cast<R*>(any.get());
		}
		else {
			_objects.resize(idx + 1);
			_objects.back() = make_any_unique<R>(std::forward<Args>(args)...);
			ptr = static_cast<R*>(_objects.back().get());
		}

		++_count.value();
		_insertion_order.push_back(idx);
		return {*ptr, true};
	}

	template<class T, class Self>
	auto try_get(this Self& self) -> CopyConst<T, Self>* {
		const auto idx = size_t{type_index<T, Domain>.value()};
		if (idx >= self._objects.size())
			return nullptr;
		return static_cast<T*>(self._objects[idx].get());
	}

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> size_t { return _count.value(); }

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto empty() const noexcept -> bool { return _count == SizeType{0}; }

	FR_FORCE_INLINE constexpr
	auto is_empty() const noexcept -> bool { return _count == SizeType{0}; }

private:
	std::vector<AnyUnique> _objects;
	std::vector<SizeType> _insertion_order;
	WithDefaultValue<SizeType{0}> _count;
};

} // namespace fr
#endif // include guard
