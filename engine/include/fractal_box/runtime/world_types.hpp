#ifndef FRACTAL_BOX_RUNTIME_WORLD_TYPES_HPP
#define FRACTAL_BOX_RUNTIME_WORLD_TYPES_HPP

#include <fmt/format.h>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/containers/sparse_set.hpp"
#include "fractal_box/core/containers/type_object_map.hpp"
#include "fractal_box/core/hashing/uni_hasher.hpp"
#include "fractal_box/core/in_place_args.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/core/memory.hpp"
#include "fractal_box/core/meta/reflection.hpp"
#include "fractal_box/core/relocation.hpp"

namespace fr {

// Core definitions
// ----------------

// World configuration types
// ^^^^^^^^^^^^^^^^^^^^^^^^^

struct WorldOpts {
	constexpr
	auto validate_or_panic() const noexcept -> bool {
		if (this->eid_bits != 32 && this->eid_bits != 64) {
			FR_PANIC_MSG("Unsupported number of EntityId bits");
		}
		if (this->eid_version_bits < 6) {
			FR_PANIC_MSG("Unsupported number of EntityId version bits");
		}
		if (this->eid_version_bits > this->eid_bits - 15) {
			FR_PANIC_MSG("Unsupported number of EntityId version bits");
		}
		return true;
	}

public:
	int eid_bits = 32;
	int eid_version_bits = default_sparse_version_bit_width<32>;
};

struct WorldConfig {
	size_t reserved_memory_limit = 128zu << 20u; // 128 MiB
};

template<WorldOpts Opts>
class World;

template<class T>
inline constexpr auto is_world = false;

template<WorldOpts Opts>
inline constexpr auto is_world<World<Opts>> = true;

template<class T>
concept c_world = is_world<T>;

// "Special" type detection"
// ^^^^^^^^^^^^^^^^^^^^^^^^^

namespace detail {

/// @note Specialized later to resolve circular dependency between `c_component` and "special" param
/// types
/// @note Do NOT specialize unless it's engine code
template<class T>
inline constexpr auto is_special_world_type = false;

template<WorldOpts Opts>
inline constexpr auto is_special_world_type<World<Opts>> = false;

} // namespace detail

template<class T>
concept c_special_world_type = detail::is_special_world_type<T>;

// EntityId
// ^^^^^^^^

template<int BitWidth, int VersionBitWidth>
requires (VersionBitWidth < BitWidth)
class EntityId {
public:
	using RawType = UIntOfSize<static_cast<size_t>(BitWidth / CHAR_BIT)>;
	using Traits = BitmaskSparseKeyTraits<RawType, VersionBitWidth>;

	static constexpr auto max_location = Traits::max_location;
	static constexpr auto max_version = Traits::max_version;

	EntityId() = default;

	FR_FORCE_INLINE constexpr
	EntityId(RawType location, RawType version) noexcept:
		_value{Traits::make(location, version)}
	{ }

	FR_FORCE_INLINE constexpr
	EntityId(NewInit, RawType location) noexcept:
		_value{Traits::make_new(location)}
	{ }

	FR_FORCE_INLINE constexpr
	EntityId(NextInit, EntityId prev) noexcept:
		_value{Traits::make_next(prev._value)}
	{ }

	friend
	auto operator==(EntityId, EntityId) noexcept -> bool = default;

	FR_FORCE_INLINE constexpr
	auto is_valid(this EntityId self) noexcept -> bool {
		return Traits::is_valid(self._value);
	}

	FR_FORCE_INLINE constexpr
	auto raw(this EntityId self) noexcept -> RawType {
		return self._value;
	}

	FR_FORCE_INLINE constexpr
	auto location(this EntityId self) noexcept -> RawType {
		return Traits::location(self._value);
	}

	FR_FORCE_INLINE constexpr
	auto version(this EntityId self) noexcept -> RawType {
		return Traits::version(self._value);
	}

private:
	RawType _value = Traits::null;
};

using EntityId32 = EntityId<32, default_sparse_version_bit_width<32>>;
using EntityId64 = EntityId<64, default_sparse_version_bit_width<64>>;

template<class T>
inline constexpr auto is_entity_id = false;

template<int BitWidth, int VersionBitWidth>
inline constexpr auto is_entity_id<EntityId<BitWidth, VersionBitWidth>> = true;

template<class T>
concept c_entity_id = is_entity_id<T>;

template<int BitWidth, int VersionBitWidth>
inline
auto format_as(EntityId<BitWidth, VersionBitWidth> eid) {
	return std::make_pair(eid.location(), eid.version());
}

template<int BitWidth, int VersionBitWidth>
inline constexpr auto detail::is_special_world_type<EntityId<BitWidth, VersionBitWidth>> = true;

// c_component
// ^^^^^^^^^^^

namespace detail {

template<class T>
inline constexpr auto is_special_query_param = false;

template<class T>
requires (is_special_query_param<T>)
inline constexpr auto is_special_world_type<T> = true;

template<c_in_place_args T>
inline constexpr auto is_special_world_type<T> = true;

} // namespace detail

template<class T>
concept c_component
	= c_user_object<T>
	&& c_maybe_with_inline_name<T>
	&& c_nothrow_movable<T>
	&& !c_special_world_type<T>
;

template<class T>
concept c_tag_component = c_component<T> && c_empty<T> && c_default_constructible<T>;

template<class T>
concept c_non_tag_component = c_component<T> && !c_tag_component<T>;

template<class T>
using IsTagComponent = BoolC<c_tag_component<T>>;

template<class T>
using IsNonTagComponent = BoolC<c_non_tag_component<T>>;

template<class T>
requires c_component<std::remove_pointer_t<T>>
struct With { };

template<c_component T>
class Has {
public:
	explicit FR_FORCE_INLINE constexpr
	Has(bool value) noexcept: _value{value} { }

	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return _value; }

	FR_FORCE_INLINE constexpr
	auto value() const noexcept -> bool { return _value; }

private:
	bool _value;
};

template<c_component T>
struct Without { };

template<class T>
inline constexpr auto detail::is_special_query_param<With<T>> = true;

template<class T>
inline constexpr auto detail::is_special_query_param<Has<T>> = true;

template<class T>
inline constexpr auto detail::is_special_query_param<Without<T>> = true;

template<class T>
concept c_component_or_in_place_args
	= c_component<std::remove_cvref_t<T>>
	|| c_in_place_args<std::remove_cvref_t<T>>;

// ComponentTypeIdx
// ^^^^^^^^^^^^^^^^

struct ComponentTypeIdxDomain: CustomTypeIndexDomainBase<> {
	static constexpr auto null_value = static_cast<ValueType>(-1);
};

using ComponentTypeIdx = TypeIndex<ComponentTypeIdxDomain>;

using ComponentTypeHash = HashDigest64;
using WorldHasher = UniHasherFast64;

// Index types
// ^^^^^^^^^^^

using ArchetypeIdx = uint32_t;
using EntityTableIdx = uint32_t;

class CompArchLink {
public:
	explicit constexpr
	CompArchLink(ArchetypeIdx arch_idx, size_t comp_local_idx) noexcept:
		_arch_idx{arch_idx},
		_comp_local_idx{comp_local_idx}
	{ }

	FR_FORCE_INLINE constexpr
	auto arch_idx() const noexcept -> ArchetypeIdx { return _arch_idx; }

	FR_FORCE_INLINE constexpr
	auto comp_local_idx() const noexcept -> size_t { return _comp_local_idx; }

private:
	ArchetypeIdx _arch_idx;
	size_t _comp_local_idx;
};

// World metadata types
// --------------------

class ComponentInfo {
public:
	template<c_component T>
	explicit
	ComponentInfo(InPlaceAsInit<T>) noexcept:
		_hash{type_hash64<T>},
		_type_info{&::fr::type_info<T>},
		_object_size{c_tag_component<T> ? 0zu : sizeof(T)},
		_type_id{ComponentTypeIdx::template of<T>},
		_copy_ctor_fn{&copy_ctor_thunk<T>},
		_copy_assign_fn{&copy_assign_thunk<T>},
		_move_ctor_fn{&move_ctor_thunk<T>},
		_move_assign_fn{&move_assign_thunk<T>},
		_dtor_fn{&dtor_thunk<T>},
		_relocate_fn{&relocate_thunk<T>},
		_format_fn{&format_thunk<T>}
	{
		if constexpr (c_tag_component<T>) {
			_singleton_object = make_any_unique<T>();
		}
	}

	FR_FORCE_INLINE
	auto type_info() const noexcept -> const TypeInfo& { return *_type_info; }

	FR_FORCE_INLINE
	auto object_size() const noexcept -> size_t { return _object_size; }

	FR_FORCE_INLINE
	auto is_tag() const noexcept -> bool { return _object_size == 0zu; }

	FR_FORCE_INLINE
	auto type_id() const noexcept -> ComponentTypeIdx { return _type_id; }

	FR_FORCE_INLINE
	auto hash() const noexcept { return _hash; }

	FR_FORCE_INLINE
	auto singleton_object() const noexcept -> void* { return _singleton_object.get(); }

	FR_FORCE_INLINE
	void copy_construct_one(void* dest, const void* src) const noexcept(assume_nothrow_assign) {
		_copy_ctor_fn(dest, src, 1zu);
	}

	FR_FORCE_INLINE
	void copy_construct_n(
		void* dest, const void* src, size_t n
	) const noexcept(assume_nothrow_assign) {
		_copy_ctor_fn(dest, src, n);
	}

	FR_FORCE_INLINE
	void copy_assign_one(void* dest, const void* src) const {
		_copy_assign_fn(dest, src, 1zu);
	}

	FR_FORCE_INLINE
	void copy_assign_n(void* dest, const void* src, size_t n) const {
		_copy_assign_fn(dest, src, n);
	}

	FR_FORCE_INLINE
	void move_construct_one(void* dest, void* src) const noexcept {
		_move_ctor_fn(dest, src, 1zu);
	}

	FR_FORCE_INLINE
	void move_construct_n(void* dest, void* src, size_t n) const noexcept {
		_move_ctor_fn(dest, src, n);
	}

	FR_FORCE_INLINE
	void move_assign_one(void* dest, void* src) const noexcept {
		_move_assign_fn(dest, src, 1zu);
	}

	FR_FORCE_INLINE
	void move_assign_n(void* dest, void* src, size_t n) const noexcept {
		_move_assign_fn(dest, src, n);
	}

	FR_FORCE_INLINE
	void destroy_one(void* dest) const noexcept { _dtor_fn(dest, 1zu); }

	FR_FORCE_INLINE
	void destroy_n(void* dest, size_t n) const noexcept { _dtor_fn(dest, n); }

	FR_FORCE_INLINE
	void relocate_one(void* dest, void* src) const noexcept { _relocate_fn(dest, src, 1zu); }

	FR_FORCE_INLINE
	void relocate_n(void* dest, void* src, size_t n) const noexcept { _relocate_fn(dest, src, n); }

	FR_FORCE_INLINE
	auto format(const void* ptr) const noexcept -> std::string { return _format_fn(ptr); }

	FR_FORCE_INLINE
	auto link_archetype(ArchetypeIdx arch_idx, size_t comp_local_idx) {
		// Insert the new element at a position that would maintain sorted order
		static constexpr auto by_arch_idx = [](const auto& link) { return link.arch_idx(); };
		auto it = std::ranges::upper_bound(_archetype_links, arch_idx, std::ranges::less{},
			by_arch_idx);
		_archetype_links.emplace(it, arch_idx, comp_local_idx);
		FR_ASSERT_AUDIT(std::ranges::is_sorted(_archetype_links, std::ranges::less{},
			by_arch_idx));
	}

	FR_FORCE_INLINE
	auto archetype_links() const noexcept -> const std::vector<CompArchLink>& {
		return _archetype_links;
	}

	FR_FORCE_INLINE
	auto entity_count() const noexcept -> size_t { return _entity_count; }

	FR_FORCE_INLINE
	auto increase_entity_count(size_t diff) noexcept { _entity_count += diff; }

	FR_FORCE_INLINE
	auto decrease_entity_count(size_t diff) noexcept {
		FR_ASSERT_AUDIT(_entity_count >= diff);
		_entity_count -= diff;
	}

private:
	// Thunk implementation
	// ^^^^^^^^^^^^^^^^^^^^
	// NOTE: Defined as standalone functions instead of lambdas to avoid ODR issues.
	// (Didn't they fix this gotcha?)

	using CopyCtorFn = void (*)(void* dest, const void* src, size_t n)
		noexcept(assume_nothrow_ctor);
	using CopyAssignFn = void (*)(void* dest, const void* src, size_t n)
		noexcept(assume_nothrow_assign);

	using MoveCtorFn = void (*)(void* dest, void* src, size_t n) noexcept;
	using MoveAssignFn = void (*)(void* dest, void* src, size_t n) noexcept;

	using DtorFn = void (*)(void* dest, size_t n) noexcept;
	using RelocateFn = void (*)(void* dest, void* src, size_t n) noexcept;

	using FormatFn = auto (*)(const void* ptr) -> std::string;

	template<class T>
	static
	auto move_ctor_thunk(void* dest, void* src, size_t n) noexcept {
		FR_LOG_TRACE("Move constructing '{}' from {} at {}", type_name<T>, src, dest);
		std::uninitialized_move_n(std::launder(static_cast<T*>(src)), n, static_cast<T*>(dest));
	}

	template<class T>
	static
	auto move_assign_thunk(void* dest, void* src, size_t n) noexcept {
		FR_LOG_TRACE("Move assigning '{}'[{}] from {} at {}", type_name<T>, n, src, dest);
		// Threre is no `std::move_n`
		auto* const src_arr = std::launder(static_cast<T*>(src));
		auto* const dest_arr = std::launder(static_cast<T*>(dest));
		for (auto i = 0zu; i < n; ++i)
			dest_arr[i] = std::move(src_arr[i]);
	}

	template<class T>
	static
	auto copy_ctor_thunk(void* dest, const void* src, size_t n) noexcept(assume_nothrow_ctor) {
		if constexpr (std::is_copy_constructible_v<T>) {
			FR_LOG_TRACE("Copy constructing '{}'[{}] from {} at {}", type_name<T>,
				n, src, dest);
			std::uninitialized_copy_n(std::launder(static_cast<const T*>(src)), n,
				static_cast<T*>(dest));
		}
		else
			FR_PANIC_FMT("Component '{}' is not copy constructible", type_name<T>);
	}

	template<class T>
	static
	auto copy_assign_thunk(void* dest, const void* src, size_t n) noexcept(assume_nothrow_assign) {
		if constexpr (std::is_copy_assignable_v<T>) {
			FR_LOG_TRACE("Copy assigning '{}'[{}] from {} at {}", type_name<T>, n, src,
				dest);
			std::copy_n(std::launder(static_cast<const T*>(src)), n,
				std::launder(static_cast<T*>(dest)));
		}
		else
			FR_PANIC_FMT("Component '{}' is not copy assignable", type_name<T>);
	}

	template<class T>
	static
	auto dtor_thunk(void* dest, size_t n) noexcept {
		FR_LOG_TRACE("Destroying '{}'[{}] at {}", type_name<T>, n, dest);
		std::destroy_n(std::launder(static_cast<T*>(dest)), n);
	}

	template<class T>
	static
	auto relocate_thunk(void* dest, void* src, size_t n) noexcept {
		FR_LOG_TRACE("Relocating '{}'[{}] from {} to {}", type_name<T>, n, src, dest);
		FR_ASSERT_AUDIT(dest != src);
		uninitialized_relocate_n(static_cast<T*>(src), n, static_cast<T*>(dest));
	}

	template<class T>
	static
	auto format_thunk(const void* ptr) -> std::string {
		if constexpr (fmt::formattable<T>) {
			return fmt::format("{}", *static_cast<const T*>(ptr));
		}
		else {
			return "?";
		}
	}

	// Immutable state
	// ^^^^^^^^^^^^^^^

	ComponentTypeHash _hash;
	const TypeInfo* _type_info;
	/// @note Also stored in `_type_info`, but cache here for quicker access
	size_t _object_size;
	/// @note Redundant, but makes generic code simpler
	ComponentTypeIdx _type_id;

	CopyCtorFn _copy_ctor_fn;
	CopyAssignFn _copy_assign_fn;

	MoveCtorFn _move_ctor_fn;
	MoveAssignFn _move_assign_fn;

	DtorFn _dtor_fn;
	RelocateFn _relocate_fn;

	FormatFn _format_fn;

	AnyUnique _singleton_object;

	// Mutable state
	// ^^^^^^^^^^^^^

	/// @note Sorted in ascending order by archetype index
	std::vector<CompArchLink> _archetype_links;
	/// @brief Number of entities that have this component
	size_t _entity_count = 0zu;
};

using ComponentInfoMap = TypeObjectMap<ComponentTypeIdxDomain, ComponentInfo>;

class EntityTable {
public:
	using RowIdx = uint32_t;

	enum class Size: size_t {
		Small,
		Standard,
		Large,

		Min = Small,
		Max = Large,
	};

	static constexpr auto num_sizes = static_cast<size_t>(Size::Max) + 1zu;

	static constexpr
	FR_FORCE_INLINE
	auto size_value(Size size) noexcept -> size_t {
		switch (size) {
			case Size::Small: return 512zu;
			case Size::Standard: return 4'096zu;
			case Size::Large: return 16'384zu;
		}
		FR_UNREACHABLE();
	}

	static constexpr
	auto size_for_count(size_t table_count, size_t row_size) noexcept -> Size {
		if (table_count <= 2zu && row_size <= 32zu)
			return Size::Small;
		if (table_count <= 20zu && row_size <= 128zu)
			return Size::Standard;
		return Size::Large;
	}

	EntityTable() = default;

	explicit
	EntityTable(
		ArchetypeIdx arch_idx,
		Size size,
		std::align_val_t buffer_alignment,
		RowIdx slot_count
	) noexcept(assume_nothrow_ctor):
		_archetype_idx{arch_idx},
		_buffer{make_aligned_buffer(size_value(size), buffer_alignment)},
		_buffer_size{size},
		_slot_count{slot_count}
	{
#if FR_ASSERT_AUDIT_ENABLED
		std::memset(_buffer.get(), 0, size_value(size));
#endif
	}

	FR_FORCE_INLINE
	auto last_row_idx() const noexcept -> RowIdx {
		FR_ASSERT_AUDIT(_alive_count.value() != 0);
		return _alive_count.value() - RowIdx{1};
	}

	FR_FORCE_INLINE
	auto increase_alive_count(RowIdx diff) noexcept -> RowIdx {
		FR_ASSERT_AUDIT(_alive_count.value() + diff <= _slot_count.value());
		FR_LOG_TRACE("Increasing table alive count from {} to {}", _alive_count.value(),
			_alive_count.value() + diff);
		_alive_count = _alive_count.value() + diff;
		return _alive_count.value();
	}

	FR_FORCE_INLINE
	auto decrease_alive_count(RowIdx diff) noexcept -> RowIdx {
		FR_ASSERT_AUDIT(_alive_count.value() >= diff);
		FR_LOG_TRACE("Decreasing table alive count from {} to {}", _alive_count.value(),
			_alive_count.value() - diff);
		_alive_count = _alive_count.value() - diff;
		return _alive_count.value();
	}

	void repurpose(
		ArchetypeIdx arch_idx,
		RowIdx slot_count
	) noexcept {
		_archetype_idx = arch_idx;
		_slot_count = slot_count;
		_alive_count.reset();
	}

	void reset() noexcept {
		_archetype_idx.reset();
		_buffer.reset();
		_slot_count.reset();
		_alive_count.reset();
		_next_idx.reset();
		_prev_idx.reset();
	}

	FR_FORCE_INLINE
	auto archetype_idx() const noexcept -> ArchetypeIdx { return _archetype_idx.value(); }

	FR_FORCE_INLINE
	auto buffer() noexcept -> std::byte* { return _buffer.get(); }

	FR_FORCE_INLINE
	auto buffer() const noexcept -> const std::byte* { return _buffer.get(); }

	FR_FORCE_INLINE
	auto buffer_size() const noexcept -> Size { return _buffer_size; }

	FR_FORCE_INLINE
	auto buffer_alignment() const noexcept -> std::align_val_t {
		return _buffer.get() ? _buffer.get_deleter().alignment() : std::align_val_t{};
	}

	FR_FORCE_INLINE
	auto slot_count() const noexcept -> RowIdx { return _slot_count.value(); }

	FR_FORCE_INLINE
	auto alive_count() const noexcept -> RowIdx { return _alive_count.value(); }

	FR_FORCE_INLINE
	auto available_count() const noexcept -> RowIdx {
		return _slot_count.value() - _alive_count.value();
	}

	FR_FORCE_INLINE
	auto has_available() const noexcept -> bool { return available_count() != 0; }

	FR_FORCE_INLINE
	auto is_empty() const noexcept -> bool { return _alive_count == RowIdx{0}; }

	FR_FORCE_INLINE
	auto has_prev_idx() const noexcept -> bool { return !_prev_idx.is_default(); }

	FR_FORCE_INLINE
	auto prev_idx() const noexcept -> EntityTableIdx { return _prev_idx.value(); }

	FR_FORCE_INLINE
	void set_prev_idx(EntityTableIdx prev) noexcept { _prev_idx = prev; }

	FR_FORCE_INLINE
	void clear_prev_idx() noexcept { _prev_idx.reset(); }

	FR_FORCE_INLINE
	auto has_next_idx() const noexcept -> bool { return !_next_idx.is_default(); }

	FR_FORCE_INLINE
	auto next_idx() const noexcept -> EntityTableIdx { return _next_idx.value(); }

	FR_FORCE_INLINE
	void set_next_idx(EntityTableIdx next) noexcept { _next_idx = next; }

	FR_FORCE_INLINE
	void clear_next_idx() noexcept { _next_idx.reset(); }

private:
	WithDefaultValue<npos_for<ArchetypeIdx>> _archetype_idx;
	AlignedBuffer _buffer;
	/// @brief Size in bytes
	Size _buffer_size;
	/// @brief Maximum number of entities that can be stored in this table
	WithDefault<RowIdx, 0> _slot_count;
	/// @brief Number of alive entities
	WithDefault<RowIdx, 0> _alive_count;
	WithDefaultValue<npos_for<EntityTableIdx>> _next_idx;
	WithDefaultValue<npos_for<EntityTableIdx>> _prev_idx;
};

inline constexpr
auto to_string_view(EntityTable::Size size) noexcept {
	using enum EntityTable::Size;
	switch (size) {
		case Small: return "Small";
		case Standard: return "Standard";
		case Large: return "Large";
	}
	FR_UNREACHABLE();
}

template<c_entity_id TEntityId>
class Archetype {
public:
	static constexpr auto neutral_hash = ComponentTypeHash{0};

	static FR_FORCE_INLINE constexpr
	auto hash_add(
		ComponentTypeHash accum, ComponentTypeHash elem
	) noexcept -> ComponentTypeHash {
		FR_PANIC_CHECK(elem != neutral_hash);
		return accum ^ elem;
	}

	static FR_FORCE_INLINE constexpr
	auto hash_remove(
		ComponentTypeHash accum, ComponentTypeHash elem
	) noexcept -> ComponentTypeHash {
		FR_PANIC_CHECK(elem != neutral_hash);
		/// @note Assuming that `accum` has exactly one 'elem' mixed in
		return accum ^ elem;
	}

	explicit constexpr
	Archetype(EmptyInit) noexcept { }

	explicit
	Archetype(
		ComponentTypeHash hash,
		ComponentTypeIdx component_id,
		const ComponentInfoMap& component_infos
	):
		_hash{hash},
		_component_ids{component_id}
	{
		FR_ASSERT(hash == WorldHasher{}(component_id));
		update_all_offsets(component_infos);
	}

	explicit
	Archetype(
		ComponentTypeHash hash,
		std::vector<ComponentTypeIdx>&& component_ids,
		const ComponentInfoMap& component_infos
	):
		_hash{hash},
		_component_ids(std::move(component_ids))
	{
		std::ranges::sort(_component_ids);
		update_all_offsets(component_infos);
	}

	FR_FORCE_INLINE
	auto add_table(EntityTableIdx new_table_idx) {
		_table_idxs.push_back(new_table_idx);
	}

	auto remove_table(EntityTableIdx table_idx) {
		const auto it = std::ranges::find(_table_idxs, table_idx);
		FR_ASSERT(it != _table_idxs.end());
		*it = _table_idxs.back();
		_table_idxs.pop_back();
	}

	/// @return Local index of the component with the given id, or `npos` if such component was not
	/// found
	FR_FORCE_INLINE
	auto find_component_local_idx(
		ComponentTypeIdx comp_id
	) const noexcept -> WithDefault<size_t, npos> {
		// NOTE: `std::binary_search` doesn't give iterator or index
		const auto it = std::ranges::lower_bound(_component_ids, comp_id);
		if (it != _component_ids.end() && *it == comp_id)
			return {static_cast<size_t>(it - _component_ids.begin())};
		return {};
	}

	FR_FORCE_INLINE
	auto has_component(ComponentTypeIdx comp_id) const noexcept -> bool {
		return !find_component_local_idx(comp_id).is_default();
	}

	FR_FORCE_INLINE
	auto hash() const noexcept -> ComponentTypeHash { return _hash; }

	FR_FORCE_INLINE
	auto component_ids() const noexcept -> const std::vector<ComponentTypeIdx>& {
		return _component_ids;
	}

	FR_FORCE_INLINE
	auto common_alignment() const noexcept -> std::align_val_t {
		return static_cast<std::align_val_t>(_common_alignment);
	}

	FR_FORCE_INLINE
	auto row_size_bytes() const noexcept -> size_t { return _row_size_bytes; }

	auto offset(EntityTable::Size table_size, size_t comp_local_idx) const noexcept -> size_t {
		FR_ASSERT_AUDIT(comp_local_idx < _component_ids.size());
		return _offsets[static_cast<size_t>(table_size)][comp_local_idx];
	}

	FR_FORCE_INLINE
	auto slot_count(EntityTable::Size table_size) const noexcept -> EntityTable::RowIdx {
		// TODO: This estimation is overly pessimistic. Can we calculate it more accurately?
		const auto usable_space = EntityTable::size_value(table_size)
			- _component_ids.size() * _common_alignment;
		const auto count = usable_space / _row_size_bytes;
		return static_cast<EntityTable::RowIdx>(count);
	}

	FR_FORCE_INLINE
	auto table_idxs() const noexcept -> const std::vector<EntityTableIdx>& {
		return _table_idxs;
	}

	FR_FORCE_INLINE
	auto has_list_head() const noexcept -> bool {
		return _table_list_head != npos_for<EntityTableIdx>;
	}

	FR_FORCE_INLINE
	auto list_head() const noexcept -> EntityTableIdx { return _table_list_head; }

	FR_FORCE_INLINE
	void set_list_head(EntityTableIdx table_idx) noexcept { _table_list_head = table_idx; }

	FR_FORCE_INLINE
	auto has_list_tail() const noexcept -> bool {
		return _table_list_tail != npos_for<EntityTableIdx>;
	}

	FR_FORCE_INLINE
	auto list_tail() const noexcept -> EntityTableIdx { return _table_list_tail; }

	FR_FORCE_INLINE
	void set_list_tail(EntityTableIdx table_idx) noexcept { _table_list_tail = table_idx; }

private:
	void update_all_offsets(const ComponentInfoMap& infos) {
		_row_size_bytes = sizeof(TEntityId);
		_common_alignment = alignof(std::max_align_t);
		for (const auto comp_idx : _component_ids) {
			const auto& comp_info = infos.get_at_unchecked(comp_idx);
			if (comp_info.is_tag())
				continue;
			const auto& type_info = comp_info.type_info();
			_common_alignment = std::max(_common_alignment, type_info.alignment());
			_row_size_bytes += type_info.size();
		}

		const auto update_for_size = [&] (EntityTable::Size table_size, auto& offsets) {
			// Reserve space for lookback ids
			auto curr_offset = slot_count(table_size) * sizeof(TEntityId);
			const auto num_slots = slot_count(table_size);
			offsets.resize(_component_ids.size());
			for (auto i = 0uz; i < _component_ids.size(); ++i) {
				const auto comp_idx = _component_ids[i];
				const auto& comp_info = infos.get_at_unchecked(comp_idx);
				if (comp_info.is_tag())
					continue;
				const auto& type_info = comp_info.type_info();
				curr_offset += aligned_up_offset(curr_offset, type_info.alignment());
				offsets[i] = curr_offset;
				curr_offset += num_slots * type_info.size();
			}
		};

		for (auto size = 0zu; size < EntityTable::num_sizes; ++size) {
			update_for_size(static_cast<EntityTable::Size>(size), _offsets[size]);
		}
	}

private:
	/// @brief XORed hashes of all the components
	/// TODO: create a separate map Hash=>ArchetypeIdx stored in the World
	ComponentTypeHash _hash {};
	/// @brief List of all components sorted in the ascending order
	std::vector<ComponentTypeIdx> _component_ids;
	size_t _common_alignment = alignof(std::max_align_t);
	/// @brief Total size of components, plus the size of lookback id
	size_t _row_size_bytes = sizeof(TEntityId);
	/// @brief Component array offsets in the buffers
	std::vector<size_t> _offsets[EntityTable::num_sizes];
	std::vector<EntityTableIdx> _table_idxs;
	EntityTableIdx _table_list_head = npos_for<EntityTableIdx>;
	EntityTableIdx _table_list_tail = npos_for<EntityTableIdx>;
};

template<c_component... Components>
requires (sizeof...(Components) > 0zu)
class WorldInsertResult {
	using ComponentList = MpList<Components...>;

public:
	WorldInsertResult() = default;

	FR_FORCE_INLINE constexpr
	WorldInsertResult(
		const SimpleArray<bool, sizeof...(Components)>& was_inserted,
		const std::tuple<Components*...>& components
	) noexcept:
		_refs{components},
		_was_inserted{was_inserted}
	{ }

	FR_FORCE_INLINE constexpr
	WorldInsertResult(
		bool was_inserted,
		Components*... component
	) noexcept
	requires (sizeof...(Components) == 1zu):
		_refs{component...},
		_was_inserted{was_inserted}
	{ }

	template<size_t I>
	requires (I < sizeof...(Components))
	FR_FORCE_INLINE constexpr FR_FLATTEN
	auto get() const noexcept -> MpPackAt<I, Components...>* { return std::get<I>(_refs); }

	template<class T>
	requires c_mp_contains_once<ComponentList, T>
	FR_FORCE_INLINE constexpr FR_FLATTEN
	auto get() const noexcept -> T* { return std::get<T*>(_refs); }

	FR_FORCE_INLINE constexpr FR_FLATTEN
	auto get() const noexcept
	requires (sizeof...(Components) == 1zu) { return std::get<0>(_refs); }

	template<size_t I>
	requires (I < sizeof...(Components))
	FR_FORCE_INLINE constexpr
	auto was_inserted() const noexcept -> bool { return _was_inserted[I]; }

	FR_FORCE_INLINE constexpr
	auto was_inserted(size_t i) const noexcept -> bool { return _was_inserted[i]; }

	template<class T>
	requires c_mp_contains_once<ComponentList, T>
	FR_FORCE_INLINE constexpr
	auto was_inserted() const noexcept -> bool {
		return _was_inserted[mp_find<ComponentList, T>];
	}

	FR_FORCE_INLINE constexpr
	auto was_inserted() const noexcept -> bool
	requires (sizeof...(Components) == 1zu) { return _was_inserted[0]; }

	inline constexpr
	auto inserted_count() const noexcept -> size_t {
		return static_cast<size_t>(std::ranges::count(_was_inserted, true));
	}

	inline constexpr
	auto inserted_all() const noexcept -> bool {
		return std::ranges::all_of(_was_inserted, Identity{});
	}

private:
	std::tuple<Components*...> _refs {};
	SimpleArray<bool, sizeof...(Components)> _was_inserted {};
};

} // namespace fr

template<class... Components>
struct std::tuple_size<fr::WorldInsertResult<Components...>>:
	std::integral_constant<size_t, sizeof...(Components)>
{ };

template<size_t I, class... Components>
struct std::tuple_element<I, fr::WorldInsertResult<Components...>> {
	using type = fr::MpPackAt<I, Components...>*;
};

#endif // include guard
