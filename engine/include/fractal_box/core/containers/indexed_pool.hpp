#ifndef FRACTAL_BOX_CORE_CONTAINERS_UNSTABLE_POOL_HPP
#define FRACTAL_BOX_CORE_CONTAINERS_UNSTABLE_POOL_HPP

#include <type_traits>
#include <variant>
#include <vector>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T, template<class> class Container, bool IsConst>
class IndexedPoolIter {
	using Cell = std::variant<T, typename Container<T>::size_type>;
	using CellContainer = AddConstIf<Container<Cell>, IsConst>;

public:
	using SizeType = typename CellContainer::size_type;

	using iterator_concept = std::forward_iterator_tag;
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = typename CellContainer::difference_type;
	using reference = typename MpLazyIf<IsConst>::template Type<const T&, T&>;
	using pointer = typename MpLazyIf<IsConst>::template Type<const T*, T*>;

	IndexedPoolIter() = default;

	explicit FR_FORCE_INLINE constexpr
	IndexedPoolIter(CellContainer& cells, SizeType idx) noexcept:
		_cells{std::addressof(cells)},
		_idx{idx}
	{ }

	explicit(false) FR_FORCE_INLINE constexpr
	operator IndexedPoolIter<T, Container, true>() noexcept
	requires (!IsConst) {
		return IndexedPoolIter<T, Container, true>{_cells, _idx};
	}

	friend constexpr
	auto operator==(IndexedPoolIter lhs, IndexedPoolIter rhs) noexcept -> bool {
		FR_ASSERT_AUDIT(lhs._cells == rhs._cells);
		return lhs._idx == rhs._idx;
	}

	FR_FORCE_INLINE constexpr
	auto operator*() const -> reference {
		auto& cell = (*_cells)[_idx];
		if (!std::holds_alternative<T>(cell))
			FR_UNREACHABLE_AUDIT();
		return std::get<T>(cell);
	}

	FR_FORCE_INLINE constexpr
	auto operator->() const -> pointer {
		return std::addressof(operator*());
	}

	constexpr
	auto operator++() noexcept -> IndexedPoolIter& {
		const auto end_idx = _cells->size();
		FR_ASSERT_AUDIT(_idx != end_idx);
		++_idx;
		while (_idx != end_idx && !std::holds_alternative<T>((*_cells)[_idx]))
			++_idx;
		return *this;
	}

	FR_FORCE_INLINE constexpr
	auto operator++(int) noexcept -> IndexedPoolIter {
		auto tmp = *this;
		operator++();
		return tmp;
	}

	FR_FORCE_INLINE constexpr
	auto index() noexcept -> SizeType {
		return _idx;
	}

private:
	CellContainer* _cells;
	SizeType _idx;
};

template<class T, class SizeType>
struct IndexedPoolInsertResult {
	constexpr
	IndexedPoolInsertResult(T& object, SizeType index) noexcept:
		_object{std::addressof(object)},
		_index{index}
	{ }

	FR_FORCE_INLINE constexpr
	auto value() const noexcept -> T& {
		return *_object;
	}

	FR_FORCE_INLINE constexpr
	auto index() const noexcept -> SizeType { return _index; }

private:
	T* _object = nullptr;
	SizeType _index = npos_for<SizeType>;
};

/// @brief An object pool that doesn't provide stable pointers, but guarantees stable indices
/// @todo TODO: Rename to IndexedPool?
template<class T, template<class> class Container = std::vector>
class IndexedPool {
	using Cell = std::variant<T, typename Container<T>::size_type>;
	using CellContainer = Container<Cell>;
	static_assert(std::random_access_iterator<typename CellContainer::iterator>);

public:
	using ValueType = T;
	using SizeType = typename CellContainer::size_type;
	using Iterator = IndexedPoolIter<T, Container, false>;
	using ConstIterator = IndexedPoolIter<T, Container, true>;
	using InsertResult = IndexedPoolInsertResult<T, SizeType>;
	inline static constexpr auto npos_idx = static_cast<typename CellContainer::size_type>(-1);

	using value_type = ValueType;
	using size_type = SizeType;
	using difference_type = typename CellContainer::difference_type;
	using allocator_type = typename CellContainer::allocator_type;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = Iterator;
	using const_iterator = ConstIterator;

	IndexedPool() = default;

	IndexedPool(FromCapacity<SizeType> capacity)
	requires requires(CellContainer cells) { cells.reserve(capacity.value); } {
		_cells.reserve(capacity.value);
	}

	constexpr
	void swap(IndexedPool& other)
	noexcept(std::is_nothrow_swappable_v<CellContainer>) {
		using std::swap;
		swap(_cells, other._cells);
		swap(_count, other._count);
		swap(_head_idx, other._head_idx);
		swap(_first_obj_idx, other._first_obj_idx);
	}

	friend FR_FORCE_INLINE constexpr
	void swap(IndexedPool& lhs, IndexedPool& rhs)
	noexcept(std::is_nothrow_swappable_v<CellContainer>) {
		lhs.swap(rhs);
	}

	FR_FORCE_INLINE constexpr
	auto insert(const T& value) -> InsertResult { return emplace(value); }

	FR_FORCE_INLINE constexpr
	auto insert(T&& value) -> InsertResult { return emplace(std::move(value)); }

	template<class... Args>
	requires std::constructible_from<T, Args...>
	constexpr
	auto emplace(Args&&... args) -> InsertResult {
		if (_count == _cells.size()) {
			const auto idx = _cells.size();
			auto& cell = _cells.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
			++_count.value();

			auto* obj = std::get_if<T>(&cell);
			if (!obj)
				FR_UNREACHABLE_AUDIT();
			return IndexedPoolInsertResult<T, SizeType>{*obj, idx};
		}
		{
			// At this point we know there are free cells, so `_head_idx` must point to *something*
			// FR_ASSERT_AUDIT(_head_idx != npos_idx);

			const auto idx = _head_idx.value();
			auto& old_head = _cells[_head_idx.value()];

			auto* next = std::get_if<SizeType>(&old_head);
			if (!next)
				FR_UNREACHABLE_AUDIT();
			_head_idx = *next;

			auto& ref = old_head.template emplace<T>(std::forward<Args>(args)...);

			if (_count == SizeType{0} || idx < _first_obj_idx)
				_first_obj_idx = idx;
			++_count.value();

			return {ref, idx};
		}
	}

	constexpr
	auto erase(SizeType idx) noexcept -> SizeType {
		if (idx >= _cells.size())
			return SizeType{0};

		auto& cell = _cells[idx];
		if (!std::holds_alternative<T>(cell))
			return SizeType{0};
		cell.template emplace<SizeType>(_head_idx.value());

		if (idx == _first_obj_idx)
			_first_obj_idx = (++Iterator{_cells, _first_obj_idx.value()}).index();
		_head_idx = idx;
		--_count.value();

		return SizeType{1};
	}

	constexpr
	void unsafe_erase(SizeType idx) noexcept {
		// See https://godbolt.org/z/GeszTjc5Y why we can't just call `try_erase`
		FR_ASSERT_AUDIT(idx < _cells.size());

		auto& cell = _cells[idx];
		if (!std::holds_alternative<T>(cell))
			FR_UNREACHABLE_AUDIT();
		cell.template emplace<SizeType>(_head_idx.value());

		if (idx == _first_obj_idx)
			_first_obj_idx = (++Iterator{_cells, _first_obj_idx.value()}).index();
		_head_idx = idx;
		--_count.value();
	}

	constexpr
	auto contains(SizeType idx) const noexcept -> bool {
		return SizeType{0} <= idx && idx < _cells.size()
			&& std::holds_alternative<T>(_cells[idx]);
	}

	template<class Self>
	constexpr
	auto try_get(this Self& self, SizeType idx) noexcept -> CopyConst<T, Self>* {
		if (idx >= self._cells.size())
			return nullptr;
		return std::get_if<T>(&self._cells[idx]);
	}

	template<class Self>
	constexpr
	auto unsafe_get(this Self& self, SizeType idx) noexcept -> CopyConst<T, Self>& {
		auto* const obj = std::get_if<T>(&self._cells[idx]);
		if (!obj)
			FR_UNREACHABLE_AUDIT();

		return *obj;
	}

	template<class Self>
	auto get(this Self& self, SizeType idx) noexcept -> CopyConst<T, Self>& {
		auto* const ptr = std::get_if<T>(&self._cells[idx]);
		FR_PANIC_CHECK(ptr);
		return *ptr;
	}

	constexpr
	auto shrink_to_fit()
	requires requires(CellContainer cells) { cells.shrink_to_fit(); } {
		_cells.shrink_to_fit();
		// TODO: It should be also possible to cut off some number of cells from the back in case
		// they are all free (without gaps)
	}

	constexpr
	auto clear() noexcept {
		_cells.clear();
		_count.reset();
		_head_idx.reset();
		_first_obj_idx.reset();
	}

	static FR_FORCE_INLINE constexpr
	auto max_size() noexcept { return CellContainer::max_size(); }

	FR_FORCE_INLINE constexpr
	auto size() const noexcept -> SizeType { return _count.value();}

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto empty() const noexcept -> bool { return _count == SizeType{0}; }

	FR_FORCE_INLINE constexpr
	auto is_empty() const noexcept -> bool { return _count == SizeType{0}; }

	FR_FORCE_INLINE constexpr
	auto cbegin() const noexcept -> ConstIterator {
		return ConstIterator{_cells, _first_obj_idx};
	}

	FR_FORCE_INLINE constexpr
	auto begin() const noexcept -> ConstIterator {
		return ConstIterator{_cells.begin(), _first_obj_idx.value()};
	}

	FR_FORCE_INLINE constexpr
	auto begin() noexcept -> Iterator {
		return Iterator{_cells.begin(), _first_obj_idx.value()};
	}

	FR_FORCE_INLINE constexpr
	auto cend() const noexcept -> ConstIterator {
		return ConstIterator{_cells.end(), _cells.end()};
	}

	FR_FORCE_INLINE constexpr
	auto end() const noexcept -> ConstIterator {
		return ConstIterator{_cells.end(), _cells.end()};
	}

	FR_FORCE_INLINE constexpr
	auto end() noexcept -> Iterator {
		return Iterator{_cells.end(), _cells.end()};
	}

private:
	CellContainer _cells;
	WithDefaultValue<SizeType{0}> _count;
	WithDefaultValue<SizeType{0}> _head_idx;
	/// @note Necessary to make `begin()` O(1), as required by the standard
	WithDefaultValue<SizeType{0}> _first_obj_idx;
};

} // namespace fr
#endif // include guard
