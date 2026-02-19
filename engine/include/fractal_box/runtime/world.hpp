#ifndef FRACTAL_BOX_RUNTIME_WORLD_HPP
#define FRACTAL_BOX_RUNTIME_WORLD_HPP

#include <algorithm>
#include <memory>
#include <utility>

#include <fmt/ranges.h>

#include "fractal_box/core/containers/simple_array.hpp"
#include "fractal_box/core/containers/type_object_map.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/in_place_args.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/core/type_index.hpp"
#include "fractal_box/runtime/world_types.hpp"
#include "fractal_box/runtime/queries.hpp"

namespace fr {

/// @todo TODO: Provide at least basic exception guarantee on all functions
template<WorldOpts Opts = {}>
class World {
	class EntityRecord;

public:
	using Entity = EntityId<Opts.eid_bits, Opts.eid_version_bits>;
	using Arch = Archetype<Entity>;

	static constexpr auto opts = Opts;
	static_assert(Opts.validate_or_panic());

	World(): World(WorldConfig{}) { }

	explicit
	World(const WorldConfig& config):
		_reserved_memory_limit{config.reserved_memory_limit}
	{ }

	World(const World&) = delete;
	auto operator=(const World&) = delete;

	World(World&&) noexcept = default;
	auto operator=(World&&) noexcept -> World& = default;

	~World() {
		destroy_all_tables();
	}

	auto spawn() noexcept(assume_nothrow_ctor) -> Entity {
		FR_PANIC_CHECK(_entity_records.size() <= size_t{Entity::max_location});

		Entity new_id;
		if (_dead_entities.empty()) {
			const auto record = EntityRecord::make_empty(0);
			_entity_records.push_back(record);
			new_id = {
				static_cast<Entity::RawType>(_entity_records.size() - 1zu),
				record.version()
			};
		}
		else {
			new_id = {next_init, _dead_entities.back()};
			_dead_entities.pop_back();
			_entity_records[new_id.location()] = EntityRecord::make_empty(new_id.version());
		}
		++_alive_entity_count;
		++_empty_entity_count;

		FR_LOG_TRACE("Spawned entity {}", new_id);
		return new_id;
	}

	template<c_component_or_in_place_args... Args>
	auto spawn_with(Args&&... args) noexcept(assume_nothrow_ctor) -> Entity {
		const auto eid = spawn();
		emplace_components(eid, std::forward<Args>(args)...);
		return eid;
	}

	/// @return `true` on success, `false` if the entity doesn't exist
	auto despawn(Entity eid) noexcept -> bool {
		FR_LOG_TRACE("Despawning entity {}", eid);
		if (!remove_all_components(eid))
			return false;

		auto& record = _entity_records[eid.location()];
		if (record.is_empty())
			--_empty_entity_count;
		record.set_dead();
		_dead_entities.push_back(eid);
		--_alive_entity_count;

		return true;
	}

	FR_FORCE_INLINE
	auto contains(Entity eid) const noexcept -> bool {
		return find_alive_record(eid) != nullptr;
	}

	// Component operations
	// ^^^^^^^^^^^^^^^^^^^^

	template<c_component Component>
	auto has_component(Entity eid) const noexcept -> bool {
		const auto* const record = find_alive_record(eid);
		if (!record || record->is_empty())
			return false;

		const auto& arch = _archetypes[record->archetype_idx()];
		return arch.has_component(ComponentTypeIdx::of<Component>);
	}

	auto component_count(Entity eid) const noexcept -> size_t {
		const auto* const record = find_alive_record(eid);
		if (!record || record->is_empty())
			return 0zu;

		const auto& arch = _archetypes[record->archetype_idx()];
		return arch.component_ids().size();
	}

	template<c_component Component, class Self>
	auto try_get_component(
		this Self& self, Entity eid
	) noexcept -> CopyConst<Component, Self>* {
		auto* const record = self.find_alive_record(eid);
		if (!record || record->is_empty())
			return nullptr;

		auto& table = self._tables[record->table_idx()];
		if (record->row_idx() >= table.alive_count())
			return nullptr;

		const auto& arch = self._archetypes[record->archetype_idx()];
		return self.template try_get_component_impl<Component>(*record, arch, table);
	}

	/// @pre `this->is_alive(eid) && this->has_component<Component>(eid)`
	template<c_component Component, class Self>
	auto get_component(this Self& self, Entity eid) noexcept -> CopyConst<Component, Self>& {
		auto* const comp = self.template try_get_component<Component>(eid);
		FR_PANIC_CHECK(comp);
		return *comp;
	}

	template<c_component... Components, class Self>
	auto try_get_components(
		this Self& self, Entity eid
	) noexcept -> std::tuple<CopyConst<Components, Self>*...> {
		auto* const record = self.find_alive_record(eid);
		if (!record || record->is_empty())
			return {};

		auto& table = self._tables[record->table_idx()];
		if (record->row_idx() >= table.alive_count())
			return {};

		const auto& arch = self._archetypes[record->archetype_idx()];
		return {self.template try_get_component_impl<Components>(*record, arch, table)...};
	}

	template<c_component Component, class... Args>
	auto emplace_component(
		Entity eid, Args&&... args
	) noexcept(assume_nothrow_ctor) -> WorldInsertResult<Component> {
		auto* const record = find_alive_record(eid);
		if (!record)
			return {};
		const auto old_record = *record;
		FR_LOG_TRACE("Emplacing component '{}' at entity {}", type_name<Component>, eid);

		const auto the_comp_id = ComponentTypeIdx::of<Component>;
		register_component_types<Component>();
		auto& the_comp_info = _component_infos.get_at_unchecked(the_comp_id);

		// TODO: Support multiple components of the same type per entity
		auto* p_old_arch = old_record.is_empty() ? nullptr
			: &_archetypes[old_record.archetype_idx()];
		if (p_old_arch && p_old_arch->has_component(the_comp_id)) {
			return {false, try_get_component_impl<Component>(old_record, *p_old_arch,
				_tables[old_record.table_idx()])};
		}

		const auto new_arch_idx = [&] -> ArchetypeIdx {
			const auto the_comp_hash = WorldHasher{}(the_comp_id);
			const auto new_arch_hash = p_old_arch
				? Arch::hash_add(p_old_arch->hash(), the_comp_hash) : the_comp_hash;

			if (auto idx = find_archetype_by_hash(new_arch_hash); !idx.is_default())
				return idx.value();

			// Create an archetype
			if (p_old_arch) {
				auto component_ids = std::vector<ComponentTypeIdx>{};
				component_ids.reserve(p_old_arch->component_ids().size() + 1zu);
				component_ids = p_old_arch->component_ids();
				component_ids.push_back(the_comp_id);
				return register_archetype_impl(new_arch_hash, std::move(component_ids));
			}
			return register_archetype_impl(new_arch_hash, {{the_comp_id}});
		}();

		const auto new_table_idx = ensure_table_for_insertion(new_arch_idx);
		auto& new_table = _tables[new_table_idx];
		auto& new_arch = _archetypes[new_arch_idx];
		const auto new_comp_local_idx = new_arch.find_component_local_idx(the_comp_id).value();
		const auto new_row_idx = new_table.alive_count() - EntityTable::RowIdx{1};

		the_comp_info.increase_entity_count(1u);
		// Create and initialize a component object
		auto* the_comp_ptr = [&] -> Component* {
			if constexpr (c_tag_component<Component>) {
				return static_cast<Component*>(the_comp_info.singleton_object());
			}
			else {
				return construct_component_impl<Component>(new_arch, new_comp_local_idx,
					new_table, new_row_idx, std::forward<Args>(args)...);
			}
		}();

		construct_lookback_id_impl(new_table, new_row_idx, eid);

		if (old_record.is_empty()) {
			--_empty_entity_count;
		}
		else {
			relocate_components_to_larger_impl(old_record, new_arch, new_table, new_row_idx);
			auto& old_table = _tables[old_record.table_idx()];
			try_plug_lookback_id_hole(old_table, old_record);
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);
		}

		record->set_indices(new_arch_idx, new_table_idx, new_row_idx);

		return {true, the_comp_ptr};
	}

	template<c_component_or_in_place_args... Args>
	requires (sizeof...(Args) > 0zu)
	auto emplace_components(
		Entity eid, Args&&... args
	) noexcept(assume_nothrow_ctor) -> WorldInsertResult<UnwrapInPlaceArgs<Args>...> {
		auto* const record = find_alive_record(eid);
		if (!record)
			return {};
		const auto old_record = *record;
		const auto* p_old_arch = old_record.is_empty() ? nullptr
			: &_archetypes[old_record.archetype_idx()];
		auto* p_old_table = old_record.is_empty() ? nullptr
			: &_tables[old_record.table_idx()];
		FR_LOG_TRACE("Emplacing components {} at entity {}",
			SimpleArray<std::string_view, sizeof...(Args)>{type_name<UnwrapInPlaceArgs<Args>>...},
			eid);

		using Components = MpTransform<MpList<Args...>, UnwrapInPlaceArgs>;
		register_component_types(Components{});

		const auto can_insert_comp = SimpleArray<bool, mp_size<Components>>{
			(p_old_arch
				? !p_old_arch->has_component(ComponentTypeIdx::of<UnwrapInPlaceArgs<Args>>)
				: true)...
		};

		auto comps = [&]<size_t... Is, class... Cs>(
			std::index_sequence<Is...>,
			MpList<Cs...>
		) FR_FORCE_INLINE_L -> std::tuple<Cs*...>{
			return {(can_insert_comp[Is]
				? nullptr
				: try_get_component_impl<Cs>(old_record, *p_old_arch, *p_old_table)
			)...};
		}(std::make_index_sequence<mp_size<Components>>{}, Components{});

		if (std::ranges::none_of(can_insert_comp, Identity{}))
			return {can_insert_comp, comps};

		const auto the_comp_ids = make_component_ids_array(Components{}, can_insert_comp);
		const auto the_comps_hash = combine_component_hashes(the_comp_ids);

		const auto new_arch_idx = [&] -> ArchetypeIdx {
			const auto new_arch_hash = p_old_arch
				? Arch::hash_add(p_old_arch->hash(), the_comps_hash) : the_comps_hash;

			if (auto idx = find_archetype_by_hash(new_arch_hash); !idx.is_default())
				return idx.value();

			// Create an archetype
			auto component_ids = std::vector<ComponentTypeIdx>{};
			if (p_old_arch) {
				component_ids.reserve(p_old_arch->component_ids().size() + mp_size<Components>);
				component_ids = p_old_arch->component_ids();
			}
			else {
				component_ids.reserve(mp_size<Components>);
			}
			for (const auto id : the_comp_ids) {
				if (id)
					component_ids.push_back(id);
			}
			return register_archetype_impl(new_arch_hash, std::move(component_ids));
		}();

		const auto new_table_idx = ensure_table_for_insertion(new_arch_idx);
		auto& new_table = _tables[new_table_idx];
		auto& new_arch = _archetypes[new_arch_idx];
		const auto new_row_idx = new_table.alive_count() - EntityTable::RowIdx{1u};

		unroll<mp_size<Components>>([&]<size_t I> {
			using Comp = MpAt<Components, I>;
			if (!can_insert_comp[I])
				return;

			auto& comp_info = _component_infos.get_at_unchecked(the_comp_ids[I]);
			comp_info.increase_entity_count(1u);

			if constexpr (c_tag_component<Comp>) {
				std::get<I>(comps) = static_cast<Comp*>(
					comp_info.singleton_object());
			}
			else {
				const auto new_comp_local_idx = new_arch.find_component_local_idx(the_comp_ids[I])
					.value();

				// Create and initialize a component object
				if constexpr (is_in_place_args<std::remove_cvref_t<MpPackAt<I, Args...>>>) {
					std::get<I>(comps) = construct_component_in_place_impl<Comp>(
						new_arch, new_comp_local_idx, new_table, new_row_idx,
						mp_pack_at<I>(std::forward<Args>(args)...));
				}
				else {
					std::get<I>(comps) = construct_component_impl<Comp>(
						new_arch, new_comp_local_idx, new_table, new_row_idx,
						mp_pack_at<I>(std::forward<Args>(args)...));
				}
			}
		});

		construct_lookback_id_impl(new_table, new_row_idx, eid);

		if (old_record.is_empty()) {
			--_empty_entity_count;
		}
		else {
			relocate_components_to_larger_impl(old_record, new_arch, new_table, new_row_idx);
			auto& old_table = _tables[old_record.table_idx()];
			try_plug_lookback_id_hole(old_table, old_record);
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);
		}

		record->set_indices(new_arch_idx, new_table_idx, new_row_idx);

		return {can_insert_comp, comps};
	}

	template<class... Canary, class Component>
	FR_FORCE_INLINE
	auto add_component(
		Entity eid, Component&& component
	) noexcept(assume_nothrow_ctor) -> std::remove_cvref_t<Component>& {
		static_assert(sizeof...(Canary) == 0, "Don't explicitly supply component type");
		const auto res = emplace_component<std::remove_cvref_t<Component>>(eid,
			std::forward<Component>(component));
		FR_PANIC_CHECK(res.was_inserted());
		return *res.template get<0>();
	}

	template<c_component Component>
	auto remove_component(Entity eid) noexcept(assume_nothrow_ctor) -> bool {
		auto* const record = find_alive_record(eid);
		if (!record || record->is_empty())
			return false;
		const auto old_record = *record;
		FR_LOG_TRACE("Removing component '{}' from entity {}", type_name<Component>, eid);

		const auto the_comp_id = ComponentTypeIdx::of<Component>;

		if (!_archetypes[old_record.archetype_idx()].has_component(the_comp_id))
			return false;

		const auto new_arch_idx = [&] -> ArchetypeIdx {
			auto& old_arch = _archetypes[old_record.archetype_idx()];
			const auto the_comp_hash = WorldHasher{}(the_comp_id);
			const auto new_arch_hash = Arch::hash_remove(old_arch.hash(), the_comp_hash);

			if (auto idx = find_archetype_by_hash(new_arch_hash); !idx.is_default())
				return idx.value();

			// Create an archetype
			auto component_ids = std::vector<ComponentTypeIdx>{};
			FR_ASSERT_AUDIT(!old_arch.component_ids().empty());
			component_ids.reserve(old_arch.component_ids().size() - 1zu);
			// Copy everything except the id of the removed component
			for (const auto id : old_arch.component_ids()) {
				if (id != the_comp_id)
					component_ids.push_back(id);
			}
			return register_archetype_impl(new_arch_hash, std::move(component_ids));
		}();

		if (new_arch_idx == ArchetypeIdx{0}) { // No components left, is empty now
			auto& old_table = _tables[old_record.table_idx()];
			const auto& old_arch = _archetypes[old_record.archetype_idx()];

			try_plug_lookback_id_hole(old_table, old_record);
			if constexpr (c_non_tag_component<Component>) {
				destroy_component_and_plug_impl<Component>(old_arch, old_table,
					old_record.row_idx());
			}

			_component_infos.get_at_unchecked(the_comp_id).decrease_entity_count(1);
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);
			++_empty_entity_count;
			record->set_empty();
		}
		else {
			const auto new_table_idx = ensure_table_for_insertion(new_arch_idx);
			const auto& new_arch = _archetypes[new_arch_idx];
			auto& new_table = _tables[new_table_idx];

			const auto new_row_idx = new_table.alive_count() - EntityTable::RowIdx{1};
			construct_lookback_id_impl(new_table, new_row_idx, eid);

			const auto& old_arch = _archetypes[old_record.archetype_idx()];
			auto& old_table = _tables[old_record.table_idx()];

			try_plug_lookback_id_hole(old_table, old_record);
			if constexpr (c_non_tag_component<Component>) {
				destroy_component_and_plug_impl<Component>(old_arch, old_table,
					old_record.row_idx());
			}

			// Relocate remaining old components to the new archetype
			relocate_components_to_smaller_impl(old_record, new_arch, new_table, new_row_idx);
			_component_infos.get_at_unchecked(the_comp_id).decrease_entity_count(1);
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);

			record->set_indices(new_arch_idx, new_table_idx, new_row_idx);
		}

		return true;
	}

	/// @return Number of removed components
	template<c_component... Components>
	requires (sizeof...(Components) > 0zu
		&& (... && mp_contains_once<MpList<Components...>, Components>))
	auto remove_components(Entity eid) noexcept -> size_t {
		auto* const record = find_alive_record(eid);
		if (!record || record->is_empty())
			return 0zu;
		const auto old_record = *record;
		const auto& r_old_arch = _archetypes[old_record.archetype_idx()];
		FR_LOG_TRACE("Removing components '{}' from entity {}",
			SimpleArray<std::string_view, sizeof...(Components)>{type_name<Components>...},
			eid);

		const auto has_comp = SimpleArray<bool, sizeof...(Components)>{
			r_old_arch.has_component(ComponentTypeIdx::of<Components>)...
		};

		if (std::ranges::none_of(has_comp, Identity{}))
			return 0zu;

		const auto the_comp_ids = make_component_ids_array(mp_list<Components...>, has_comp);
		const auto the_comps_hash = combine_component_hashes(the_comp_ids);

		const auto new_arch_idx = [&] -> ArchetypeIdx {
			const auto new_arch_hash = Arch::hash_remove(r_old_arch.hash(), the_comps_hash);

			if (auto idx = find_archetype_by_hash(new_arch_hash); !idx.is_default())
				return idx.value();

			// Create an archetype
			auto component_ids = std::vector<ComponentTypeIdx>{};
			FR_ASSERT_AUDIT(!r_old_arch.component_ids().empty());
			component_ids.reserve(r_old_arch.component_ids().size() - 1zu);
			// Copy everything except the ids of the removed components
			for (const auto id : r_old_arch.component_ids()) {
				if (!std::ranges::contains(the_comp_ids, id))
					component_ids.push_back(id);
			}
			return register_archetype_impl(new_arch_hash, std::move(component_ids));
		}();

		if (new_arch_idx == ArchetypeIdx{0}) { // No components left, is empty now
			const auto& old_arch = _archetypes[old_record.archetype_idx()];
			auto& old_table = _tables[old_record.table_idx()];

			try_plug_lookback_id_hole(old_table, old_record);
			unroll<sizeof...(Components)>([&]<size_t I> FR_FORCE_INLINE_L {
				using Comp = MpPackAt<I, Components...>;
				if (!has_comp[I])
					return;

				if constexpr (c_non_tag_component<Comp>) {
					destroy_component_and_plug_impl<Comp>(old_arch, old_table,
						old_record.row_idx());
				}
				_component_infos.get_at_unchecked(the_comp_ids[I]).decrease_entity_count(1);
			});
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);

			record->set_empty();
		}
		else {
			const auto new_table_idx = ensure_table_for_insertion(new_arch_idx);
			const auto& new_arch = _archetypes[new_arch_idx];
			auto& new_table = _tables[new_table_idx];

			const auto new_row_idx = new_table.alive_count() - EntityTable::RowIdx{1};
			construct_lookback_id_impl(new_table, new_row_idx, eid);

			const auto& old_arch = _archetypes[old_record.archetype_idx()];
			auto& old_table = _tables[old_record.table_idx()];

			try_plug_lookback_id_hole(old_table, old_record);
			unroll<sizeof...(Components)>([&]<size_t I> FR_FORCE_INLINE_L {
				using Comp = MpPackAt<I, Components...>;
				if (!has_comp[I])
					return;

				if constexpr (c_non_tag_component<Comp>) {
					destroy_component_and_plug_impl<Comp>(old_arch, old_table,
						old_record.row_idx());
				}
				_component_infos.get_at_unchecked(the_comp_ids[I]).decrease_entity_count(1);
			});
			// Relocate remaining old components to the new archetype
			relocate_components_to_smaller_impl(old_record, new_arch, new_table, new_row_idx);
			table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);

			record->set_indices(new_arch_idx, new_table_idx, new_row_idx);
		}

		return static_cast<size_t>(std::ranges::count(has_comp, true));
	}

	auto remove_all_components(Entity eid) noexcept -> bool {
		auto* const record = find_alive_record(eid);
		if (!record)
			return false;
		const auto old_record = *record;

		if (old_record.is_empty())
			return true;

		auto& table = _tables[old_record.table_idx()];
		auto& arch = _archetypes[old_record.archetype_idx()];
		FR_LOG_TRACE("Removing all components from entity {} at row {} of {}", eid,
			old_record.row_idx(), table.alive_count());

		try_plug_lookback_id_hole(table, old_record);

		for (auto i = 0zu; i < arch.component_ids().size(); ++i) {
			const auto comp_id = arch.component_ids()[i];
			auto& comp_info = _component_infos.get_at_unchecked(comp_id);
			comp_info.decrease_entity_count(1zu);
			if (comp_info.is_tag())
				continue;

			auto* const comp = get_component_impl(arch, i, comp_info.object_size(), table,
				old_record.row_idx());
			comp_info.destroy_one(comp);

			// Plug holes in the table (in component arrays)
			if (old_record.row_idx() != table.last_row_idx()) {
				auto* const last_comp = get_component_impl(arch, i, comp_info.object_size(), table,
					table.last_row_idx());
				comp_info.relocate_one(comp, last_comp);
			}
		}

		record->set_empty();
		table_decrease_alive_count(old_record.archetype_idx(), old_record.table_idx(), 1u);
		++_empty_entity_count;

		return true;
	}

	void clear() noexcept {
		destroy_all_tables();
		_component_infos.clear();
		_entity_records.clear();
		_entity_records.shrink_to_fit();
		_alive_entity_count.reset();
		_empty_entity_count.reset();
		_archetypes = {Arch{empty_init}};
		_tables.clear();
		for (auto& head : _reserved_tables_heads)
			head.reset();
		for (auto& tail : _reserved_tables_tails)
			tail.reset();
		_dead_tables_head.reset();
		_dead_tables_tail.reset();
		_reserved_memory.reset();
		_dead_entities.clear();
		_dead_entities.shrink_to_fit();
	}

	// Queries
	// ^^^^^^^

	template<c_component Component>
	auto entity_count_with_component() const noexcept -> size_t {
		auto* const comp_info = _component_infos.try_get<Component>();
		return comp_info ? comp_info->entity_count() : 0zu;
	}

	FR_FORCE_INLINE
	auto entity_count() const noexcept -> size_t {
		return _alive_entity_count.value();
	}

	FR_FORCE_INLINE
	auto is_empty() const noexcept -> bool {
		return _alive_entity_count == 0zu;
	}

	auto count_entities_in_tables() const noexcept -> size_t {
		auto sum = _empty_entity_count.value();
		for (const auto& table: _tables)
			sum += table.alive_count();
		return sum;
	}

	template<class... Components>
	FR_FORCE_INLINE
	auto build_uncached_query() noexcept -> UncachedQuery<World, Components...> {
		return UncachedQuery<World, Components...>{*this};
	}

	template<class Q>
	FR_FORCE_INLINE
	auto build_query() {
		if constexpr (is_uncached_query<Q>) {
			return Q{*this};
		}
		else
			static_assert(false);
	}

	FR_FORCE_INLINE
	auto reserved_memory() const noexcept -> size_t { return _reserved_memory.value(); }

	// Mixins
	// ^^^^^^
	// TODO (?)

	// Misc
	// ^^^^

	template<c_component Components>
	FR_FORCE_INLINE
	void register_component_types() {
		if (_component_infos.try_emplace<Components>(in_place_as<Components>)) {
			FR_LOG_TRACE("Registered component type '{}'", type_name<Components>);
		}
	}

	template<c_component... Components>
	requires (sizeof...(Components) > 1)
	FR_FORCE_INLINE
	void register_component_types() {
		(..., register_component_types<Components>());
	}

	template<c_component... Components>
	FR_FORCE_INLINE
	void register_component_types(MpList<Components...>) {
		register_component_types<Components...>();
	}

private:
	template<class TWorld, c_query_param... Components>
	friend class UncachedQuery;

	// Helpers
	// ^^^^^^^

	template<class... Components>
	static FR_FORCE_INLINE
	auto make_component_ids_array(
		MpList<Components...>,
		const SimpleArray<bool, sizeof...(Components)>& mask
	) noexcept -> SimpleArray<ComponentTypeIdx, sizeof...(Components)> {
		return [&]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
			return SimpleArray<ComponentTypeIdx, sizeof...(Components)>{
				(mask[Is]
					? ComponentTypeIdx::of<MpPackAt<Is, Components...>>
					: ComponentTypeIdx{})...
			};
		}(std::make_index_sequence<sizeof...(Components)>{});
	}

	template<size_t N>
	requires (N > 0)
	static FR_FORCE_INLINE
	auto combine_component_hashes(const SimpleArray<ComponentTypeIdx, N>& ids) {
		const auto hasher = WorldHasher{};
		auto hash = Arch::neutral_hash;
		for (const auto id : ids) {
			if (id)
				hash = Arch::hash_add(hash, hasher(id));
		}
		return hash;
	}

	template<class Self>
	auto find_alive_record(
		this Self& self, Entity eid
	) noexcept -> CopyConst<EntityRecord, Self>* {
		if (!eid.is_valid())
			return nullptr;
		const auto record_idx = size_t{eid.location()};
		if (record_idx >= self._entity_records.size())
			return nullptr;
		auto& record = self._entity_records[record_idx];
		if (!record.is_alive() || record.version() != eid.version())
			return nullptr;
		return &record;
	}

	FR_FORCE_INLINE
	auto find_archetype_by_hash(
		ComponentTypeHash hash
	) noexcept -> WithDefault<ArchetypeIdx, npos_for<ArchetypeIdx>> {
		const auto it = std::ranges::find_if(_archetypes, [hash](const auto& arch) {
			return arch.hash() == hash;
		});
		if (it != _archetypes.end())
			return {static_cast<ArchetypeIdx>(it - _archetypes.begin())};
		return {};
	}

	auto make_table(
		ArchetypeIdx arch_idx,
		EntityTable::Size table_size
	) noexcept(assume_nothrow_ctor) -> EntityTableIdx {
		const auto& arch = _archetypes[arch_idx];

		[[maybe_unused]] auto& table = _tables.emplace_back(
			arch_idx,
			table_size,
			arch.common_alignment(),
			arch.slot_count(table_size)
		);
		const auto table_idx = static_cast<EntityTableIdx>(_tables.size() - 1zu);
		FR_LOG_TRACE("Created table {} of size '{}' for archetype {}", table_idx,
			to_string_view(table_size), arch_idx);

		return table_idx;
	}

	auto revive_table(
		ArchetypeIdx arch_idx,
		EntityTable::Size table_size
	) noexcept(assume_nothrow_ctor) -> EntityTableIdx {
		const auto& arch = _archetypes[arch_idx];
		const auto table_idx = dead_tables_remove_head();
		_tables[table_idx] = EntityTable{
			arch_idx,
			table_size,
			arch.common_alignment(),
			arch.slot_count(table_size)
		};
		FR_LOG_INFO("Revived table {} of size '{}' for archetype {}", table_idx,
			to_string_view(table_size), arch_idx);
		return table_idx;
	}

	void repurpose_table(ArchetypeIdx arch_idx, EntityTableIdx table_idx) noexcept {
		const auto& arch = _archetypes[arch_idx];
		auto& table = _tables[table_idx];
		table.repurpose(arch_idx, arch.slot_count(table.buffer_size()));
		FR_LOG_TRACE("Repurposed table {} for archetype {}", table_idx, arch_idx);
	}

	void available_tables_add_head(ArchetypeIdx arch_idx, EntityTableIdx table_idx) noexcept {
		auto& arch = _archetypes[arch_idx];
		auto& table = _tables[table_idx];
		FR_LOG_TRACE("Adding table {} as a list head to archetype {}", table_idx, arch_idx);
		FR_ASSERT_AUDIT(!table.has_prev_idx() && !table.has_next_idx());
		if (arch.has_list_head()) {
			_tables[arch.list_head()].set_prev_idx(table_idx);
		}

		if (!arch.has_list_tail())
			arch.set_list_tail(table_idx);
		table.set_next_idx(arch.list_head());
		arch.set_list_head(table_idx);
	}

	void available_tables_add_tail(ArchetypeIdx arch_idx, EntityTableIdx table_idx) noexcept {
		auto& arch = _archetypes[arch_idx];
		auto& table = _tables[table_idx];
		FR_LOG_TRACE("Adding table {} as a list tail to archetype {}", table_idx, arch_idx);
		if (arch.has_list_head()) {
			FR_ASSERT(arch.has_list_tail());
			auto& tail = _tables[arch.list_tail()];
			tail.set_next_idx(table_idx);
			table.set_prev_idx(arch.list_tail());
		}
		else {
			arch.set_list_head(table_idx);
		}
		arch.set_list_tail(table_idx);
	}

	void available_tables_remove(ArchetypeIdx arch_idx, EntityTableIdx table_idx) noexcept {
		auto& arch = _archetypes[arch_idx];
		auto& table = _tables[table_idx];
		FR_LOG_TRACE("Removing table {} from the list of archetype {}", table_idx, arch_idx);
		if (table.has_prev_idx()) {
			auto& prev = _tables[table.prev_idx()];
			prev.set_next_idx(table.next_idx());
		}
		else if (table_idx == arch.list_head()) {
			arch.set_list_head(table.next_idx());
		}
		else {
			FR_ASSERT(false);
		}

		if (table.has_next_idx()) {
			auto& next = _tables[table.next_idx()];
			next.set_prev_idx(table.prev_idx());
		}
		else if (table_idx == arch.list_tail()) {
			arch.set_list_tail(table.prev_idx());
		}
		else {
			FR_ASSERT(false);
		}

		table.clear_prev_idx();
		table.clear_next_idx();
	}

	void reserved_tables_add_head(EntityTableIdx table_idx) noexcept {
		auto& table = _tables[table_idx];
		auto& head_idx = _reserved_tables_heads[static_cast<size_t>(table.buffer_size())];
		auto& tail_idx = _reserved_tables_tails[static_cast<size_t>(table.buffer_size())];

		FR_LOG_TRACE("Adding table {} to the list of reserved '{}' tables", table_idx,
			to_string_view(table.buffer_size()));

		if (!head_idx.is_default()) {
			_tables[head_idx.value()].set_prev_idx(table_idx);
		}

		if (tail_idx.is_default()) {
			tail_idx = table_idx;
		}

		table.set_next_idx(head_idx.value());
		head_idx = table_idx;
		_reserved_memory = _reserved_memory.value() + EntityTable::size_value(table.buffer_size());
	}

	auto reserved_tables_remove_head(EntityTable::Size table_size) noexcept -> EntityTableIdx {
		auto& head_idx = _reserved_tables_heads[static_cast<size_t>(table_size)];
		auto& tail_idx = _reserved_tables_tails[static_cast<size_t>(table_size)];
		FR_ASSERT(!head_idx.is_default() && !tail_idx.is_default());
		const auto table_idx = head_idx.value();
		auto& table = _tables[table_idx];

		FR_LOG_TRACE("Removing table {} from the head of the list of reserved '{}' tables",
			table_idx, to_string_view(table_size));

		head_idx = table.next_idx();

		if (table.has_next_idx()) {
			auto& next = _tables[table.next_idx()];
			next.clear_prev_idx();
		}
		else if (table_idx == tail_idx.value()) {
			tail_idx.reset();
		}
		else {
			FR_ASSERT(false);
		}

		table.clear_prev_idx();
		table.clear_next_idx();
		_reserved_memory = _reserved_memory.value() - EntityTable::size_value(table_size);

		return table_idx;
	}

	void reserved_tables_remove(EntityTable::Size table_size, EntityTableIdx table_idx) noexcept {
		FR_LOG_TRACE("Removing table {} from the list of reserved '{}' tables", table_idx,
			to_string_view(table_size));

		auto& head_idx = _reserved_tables_heads[static_cast<size_t>(table_size)];
		auto& tail_idx = _reserved_tables_tails[static_cast<size_t>(table_size)];
		FR_ASSERT(!head_idx.is_default() && !tail_idx.is_default());
		auto& table = _tables[table_idx];

		if (table.has_prev_idx()) {
			auto& prev = _tables[table.prev_idx()];
			prev.set_next_idx(table.next_idx());
		}
		else if (table_idx == head_idx.value()) {
			head_idx = table.next_idx();
		}
		else {
			FR_ASSERT(false);
		}

		if (table.has_next_idx()) {
			auto& next = _tables[table.next_idx()];
			next.set_prev_idx(table.prev_idx());
		}
		else if (table_idx == tail_idx.value()) {
			tail_idx = table.prev_idx();
		}
		else {
			FR_ASSERT(false);
		}

		table.clear_prev_idx();
		table.clear_next_idx();
		_reserved_memory = _reserved_memory.value() - EntityTable::size_value(table_size);
	}

	void dead_tables_add_head(EntityTableIdx table_idx) noexcept {
		FR_LOG_TRACE("Adding table {} to the list of dead tables", table_idx);

		auto& table = _tables[table_idx];

		if (!_dead_tables_head.is_default()) {
			_tables[_dead_tables_head.value()].set_prev_idx(table_idx);
		}

		if (_dead_tables_tail.is_default()) {
			_dead_tables_tail = table_idx;
		}

		table.set_next_idx(_dead_tables_head.value());
		_dead_tables_head = table_idx;
	}

	auto dead_tables_remove_head() noexcept -> EntityTableIdx {
		FR_ASSERT(!_dead_tables_head.is_default() && !_dead_tables_tail.is_default());
		const auto table_idx = _dead_tables_head.value();
		auto& table = _tables[table_idx];

		FR_LOG_TRACE("Removing table {} from the list of dead tables", table_idx);

		_dead_tables_head = table.next_idx();

		if (table.has_next_idx()) {
			auto& next = _tables[table.next_idx()];
			next.clear_prev_idx();
		}
		else if (table_idx == _dead_tables_tail.value()) {
			_dead_tables_tail.reset();
		}
		else {
			FR_ASSERT(false);
		}

		table.clear_prev_idx();
		table.clear_next_idx();

		return table_idx;
	}

	void table_increase_alive_count(
		ArchetypeIdx arch_idx,
		EntityTableIdx table_idx,
		EntityTable::RowIdx diff
	) noexcept {
		FR_LOG_TRACE("In table {}:", table_idx);
		auto& table = _tables[table_idx];
		if (table.is_empty()) {
			reserved_tables_remove(table.buffer_size(), table_idx);
			if (table.available_count() != diff) {
				available_tables_add_head(arch_idx, table_idx);
			}
		}
		else if (table.available_count() == diff) {
			available_tables_remove(arch_idx, table_idx);
		}
		table.increase_alive_count(diff);
	}

	void table_decrease_alive_count(
		ArchetypeIdx arch_idx,
		EntityTableIdx table_idx,
		EntityTable::RowIdx diff
	) noexcept {
		FR_LOG_TRACE("In table {}:", table_idx);
		auto& table = _tables[table_idx];
		if (table.alive_count() == diff) {
			if (table.has_available()) {
				available_tables_remove(arch_idx, table_idx);
			}
			auto& arch = _archetypes[arch_idx];
			arch.remove_table(table_idx);

			if (_reserved_memory.value() + EntityTable::size_value(table.buffer_size())
				<= _reserved_memory_limit
			) {
				reserved_tables_add_head(table_idx);
			}
			else {
				table.reset();
				dead_tables_add_head(table_idx);
				return;
			}
		}
		else if (!table.has_available()) {
			available_tables_add_head(arch_idx, table_idx);
		}
		table.decrease_alive_count(diff);
	}

	/// @brief Find a table of the given archetype that has at least one available slot, or create
	/// a new one if it doesn't exist
	/// @todo TODO: Support batched insertion
	auto ensure_table_for_insertion(ArchetypeIdx arch_idx) -> EntityTableIdx {
		FR_LOG_TRACE("Ensuring table for insertion into archetype {}", arch_idx);

		auto& arch = _archetypes[arch_idx];

		if (arch.has_list_head()) {
			const auto table_idx = arch.list_head();
			FR_LOG_TRACE("- Found available table {}", table_idx);
			auto& table = _tables[table_idx];
			if (table.available_count() == 1u) {
				available_tables_remove(arch_idx, table_idx);
			}
			table.increase_alive_count(1u);
			return table_idx;
		}

		const auto table_idx = [&] {
			const auto table_size = EntityTable::size_for_count(arch.table_idxs().size() + 1zu,
				arch.row_size_bytes());
			if (const auto head_idx = _reserved_tables_heads[static_cast<size_t>(table_size)];
				!head_idx.is_default()
			) {
				auto& table = _tables[head_idx.value()];
				if (table.buffer_alignment() == arch.common_alignment()) {
					FR_LOG_TRACE("- Found empty table {}", head_idx.value());
					reserved_tables_remove_head(table_size);
					repurpose_table(arch_idx, head_idx.value());
					return head_idx.value();
				}
			}

			if (!_dead_tables_head.is_default()) {
				return revive_table(arch_idx, table_size);
			}

			return make_table(arch_idx, table_size);
		}();

		auto& table = _tables[table_idx];
		arch.add_table(table_idx);
		if (table.available_count() != 1u) {
			available_tables_add_tail(arch_idx, table_idx);
		}
		table.increase_alive_count(1u);
		return table_idx;
	}

	void print_table(EntityTableIdx table_idx) const {
		FR_LOG_TRACE("Table {}:", table_idx);
		const auto& table = _tables[table_idx];
		const auto& arch = _archetypes[table.archetype_idx()];

		fmt::print("EID\t");
		for (auto i = 0zu ; i < arch.component_ids().size(); ++i) {
			const auto comp_id = arch.component_ids()[i];
			const auto& info = _component_infos.get_at_unchecked(comp_id);
			fmt::print("{}\t", info.type_info().display_name());
		}
		fmt::print("\n");

		for (auto row = EntityTable::RowIdx{0}; row < table.alive_count(); ++row) {
			fmt::print("{}\t", get_lookback_id_impl(table, row));
			for (auto i = 0zu ; i < arch.component_ids().size(); ++i) {
				const auto comp_id = arch.component_ids()[i];
				const auto& info = _component_infos.get_at_unchecked(comp_id);
				fmt::print("{}\t", info.format(get_component_impl(arch, i, info.object_size(),
					table, row)));
			}
			fmt::print("\n");
		}
	}

	void destroy_all_tables() noexcept {
		for (auto& arch : _archetypes) {
			for (const auto table_idx : arch.table_idxs()) {
				auto& table = _tables[table_idx];
				if (table.is_empty())
					continue;
				FR_LOG_TRACE("Destroying table {}", table_idx);
				for (auto row = EntityTable::RowIdx{0}; row < table.alive_count(); ++row) {
					FR_LOG_TRACE("Despawning entity {}", get_lookback_id_impl(table, row));
				}
				for (auto i = 0zu; i < arch.component_ids().size(); ++i) {
					const auto comp_id = arch.component_ids()[i];
					const auto& info = _component_infos.get_at_unchecked(comp_id);
					info.destroy_n(get_component_impl(arch, i, info.object_size(), table, 0),
						table.alive_count());
				}
			}
		}
	}

	static FR_FORCE_INLINE
	auto construct_lookback_id_impl(
		EntityTable& table,
		EntityTable::RowIdx row_idx,
		Entity eid
	) noexcept -> Entity* {
		FR_ASSERT_AUDIT(row_idx < table.alive_count());

		auto* const ptr = reinterpret_cast<Entity*>(table.buffer() + sizeof(Entity) * row_idx);
		FR_LOG_TRACE("Constructing lookback EID {} at {}", eid, fmt::ptr(ptr));
		return std::construct_at(ptr, eid);
	}

	template<c_maybe_const_of<EntityTable> Table>
	static FR_FORCE_INLINE
	auto get_lookback_id_impl(
		Table& table,
		EntityTable::RowIdx row_idx
	) noexcept -> CopyConst<Entity, Table>& {
		FR_ASSERT_AUDIT(row_idx < table.alive_count());

		auto* const raw_ptr = table.buffer() + sizeof(Entity) * row_idx;
		return *std::launder(reinterpret_cast<CopyConst<Entity, Table>*>(raw_ptr));
	}

	auto try_plug_lookback_id_hole(EntityTable& table, EntityRecord record) noexcept -> bool {
		if (record.row_idx() == table.last_row_idx())
			return false;

		auto& hole_eid = get_lookback_id_impl(table, record.row_idx());
		auto& plug_eid = get_lookback_id_impl(table, table.last_row_idx());

		FR_LOG_TRACE("Swapping lookback EID {} at {}[{}] with {} at {}[{}]",
			hole_eid, fmt::ptr(&hole_eid), record.row_idx(), plug_eid, fmt::ptr(&plug_eid),
			table.last_row_idx());
		std::swap(hole_eid, plug_eid);

		FR_LOG_TRACE("Setting record at {} to {}:{}:{}/{}",
			hole_eid.location(),
			record.archetype_idx(),
			record.table_idx(),
			record.row_idx(),
			hole_eid.version()
		);
		_entity_records[hole_eid.location()].set_all(
			record.archetype_idx(),
			record.table_idx(),
			record.row_idx(),
			hole_eid.version()
		);

		return true;
	}

	template<c_non_tag_component Component, class... Args>
	FR_FORCE_INLINE
	auto construct_component_impl(
		const Arch& arch,
		size_t comp_local_idx,
		EntityTable& table,
		EntityTable::RowIdx row_idx,
		Args&&... args
	) noexcept(assume_nothrow_ctor) -> Component* {
		FR_ASSERT_AUDIT(row_idx < table.alive_count());
		FR_ASSERT_AUDIT(comp_local_idx != npos);
		const auto offset = arch.offset(table.buffer_size(), comp_local_idx);
		auto* const ptr = std::construct_at(
			reinterpret_cast<Component*>(table.buffer() + offset + sizeof(Component) * row_idx),
			std::forward<Args>(args)...
		);
		FR_LOG_TRACE("Constructing '{}' at {}", type_name<Component>, fmt::ptr(ptr));
		return ptr;
	}

	template<c_non_tag_component Component, class... Args>
	FR_FORCE_INLINE
	auto construct_component_in_place_impl(
		const Arch& arch,
		size_t comp_local_idx,
		EntityTable& table,
		EntityTable::RowIdx row_idx,
		const InPlaceArgs<Component, Args...>& args
	) noexcept(assume_nothrow_ctor) -> Component* {
		FR_ASSERT_AUDIT(row_idx < table.alive_count());
		FR_ASSERT_AUDIT(comp_local_idx != npos);
		const auto offset = arch.offset(table.buffer_size(), comp_local_idx);
		auto* const ptr = [&]<size_t... Is>(std::index_sequence<Is...>) {
			return std::construct_at(
				reinterpret_cast<Component*>(table.buffer() + offset + sizeof(Component) * row_idx),
				std::get<Is>(args.args)...
			);
		}(std::make_index_sequence<sizeof...(Args)>{});
		FR_LOG_TRACE("Constructing '{}' at {}", type_name<Component>, fmt::ptr(ptr));
		return ptr;
	}

	template<class Component>
	FR_FORCE_INLINE
	void destroy_component_impl(
		const Arch& arch,
		size_t comp_local_idx,
		EntityTable& table,
		EntityTable::RowIdx row_idx
	) noexcept {
		FR_ASSERT_AUDIT(comp_local_idx != npos);
		const auto offset = arch.offset(table.buffer_size(), comp_local_idx);
		auto* const ptr = std::launder(calc_component_ptr<Component>(table, offset, row_idx));

		FR_LOG_TRACE("Destroying '{}' at {}", type_name<Component>, fmt::ptr(ptr));
		std::destroy_at(ptr);
	}

	template<class Component>
	void destroy_component_and_plug_impl(
		const Arch& arch,
		EntityTable& table,
		EntityTable::RowIdx row_idx
	) noexcept {
		const auto comp_id = ComponentTypeIdx::of<Component>;
		const auto comp_local_idx = arch.find_component_local_idx(comp_id).value();

		destroy_component_impl<Component>(arch, comp_local_idx, table, row_idx);

		if (row_idx != table.last_row_idx()) {
			relocate_component_impl<Component>(arch, comp_local_idx, table, row_idx,
				table.last_row_idx());
		}
	}

	template<class Component>
	FR_FORCE_INLINE
	void relocate_component_impl(
		const Arch& arch,
		size_t comp_local_idx,
		EntityTable& table,
		EntityTable::RowIdx dest_row_idx,
		EntityTable::RowIdx src_row_idx
	) noexcept {
		FR_ASSERT_AUDIT(comp_local_idx != npos);
		FR_ASSERT_AUDIT(dest_row_idx != src_row_idx);
		const auto offset = arch.offset(table.buffer_size(), comp_local_idx);

		auto* const dest_ptr = calc_component_ptr<Component>(table, offset, dest_row_idx);
		auto* const src_ptr = std::launder(calc_component_ptr<Component>(table, offset,
			src_row_idx));

		FR_LOG_TRACE("Relocating '{}' from {} to {}", type_name<Component>,
			fmt::ptr(src_ptr), fmt::ptr(dest_ptr));
		relocate_at(dest_ptr, src_ptr);
	}

	void relocate_components_single_impl(
		const Arch& old_arch,
		EntityTable& old_table,
		EntityTable::RowIdx old_row_idx,
		const Arch& new_arch,
		EntityTable& new_table,
		EntityTable::RowIdx new_row_idx,
		size_t old_local_idx,
		size_t new_local_idx
	) noexcept {
		const auto comp_id = old_arch.component_ids()[old_local_idx];
		auto& comp_info = _component_infos.get_at_unchecked(comp_id);
		if (comp_info.is_tag())
			return;
		auto* const old_comp = get_component_impl(old_arch, old_local_idx,
			comp_info.object_size(), old_table, old_row_idx);
		auto* const new_comp = get_component_impl(new_arch, new_local_idx,
			comp_info.object_size(), new_table, new_row_idx);
		comp_info.relocate_one(new_comp, old_comp);

		// Plug a hole in the old table (in the component array)
		if (old_row_idx != old_table.last_row_idx()) {
			auto* const last_old_comp = get_component_impl(old_arch, old_local_idx,
				comp_info.object_size(), old_table, old_table.last_row_idx());
			comp_info.relocate_one(old_comp, last_old_comp);
		}
	}

	void relocate_components_to_larger_impl(
		EntityRecord old_record,
		const Arch& new_arch,
		EntityTable& new_table,
		EntityTable::RowIdx new_row_idx
	) noexcept {
		FR_ASSERT_AUDIT(!old_record.is_empty());
		auto& old_table = _tables[old_record.table_idx()];
		const auto& old_arch = _archetypes[old_record.archetype_idx()];

		// Relocate old components to the new archetype
		for (auto old_i = 0zu; old_i < old_arch.component_ids().size(); ++old_i) {
			// New archetype contains more components, so new local index is always >= old
			// local index
			auto new_i = old_i;
			while (new_arch.component_ids()[new_i] != old_arch.component_ids()[old_i]) {
				FR_ASSERT_AUDIT(new_i < new_arch.component_ids().size());
				++new_i;
			}

			relocate_components_single_impl(
				old_arch, old_table, old_record.row_idx(),
				new_arch, new_table, new_row_idx,
				old_i, new_i
			);
		}
	}

	void relocate_components_to_smaller_impl(
		EntityRecord old_record,
		const Arch& new_arch,
		EntityTable& new_table,
		EntityTable::RowIdx new_row_idx
	) noexcept {
		FR_ASSERT_AUDIT(!old_record.is_empty());
		auto& old_table = _tables[old_record.table_idx()];
		const auto& old_arch = _archetypes[old_record.archetype_idx()];
		for (auto new_i = 0zu; new_i < new_arch.component_ids().size(); ++new_i) {
			// New archetype contains fewer components, so new local index is always <= old
			// local index
			auto old_i = new_i;
			while (old_arch.component_ids()[old_i] != new_arch.component_ids()[new_i]) {
				FR_ASSERT_AUDIT(old_i < old_arch.component_ids().size());
				++old_i;
			}

			relocate_components_single_impl(
				old_arch, old_table, old_record.row_idx(),
				new_arch, new_table, new_row_idx,
				old_i, new_i
			);
		}
	}

	template<c_tag_component Component, class Self>
	FR_FORCE_INLINE
	auto get_tag_component(this Self& self) noexcept -> CopyConst<Component, Self>* {
		const auto comp_id = ComponentTypeIdx::of<Component>;
		const auto& comp_info = self._component_infos.get_at_unchecked(comp_id);
		return static_cast<CopyConst<Component, Self>*>(comp_info.singleton_object());
	}

	template<class Component>
	requires (c_non_tag_component<std::remove_const_t<Component>>)
	static FR_FORCE_INLINE
	auto calc_component_ptr(
		CopyConst<EntityTable, Component>& table,
		size_t offset,
		EntityTable::RowIdx row_idx
	) -> Component* {
		return reinterpret_cast<Component*>(table.buffer() + offset + sizeof(Component) * row_idx);
	}

	template<c_maybe_const_of<EntityTable> Table>
	static FR_FORCE_INLINE
	auto get_component_impl(
		const Arch& arch,
		size_t comp_local_idx,
		size_t object_size,
		Table& table,
		EntityTable::RowIdx row_idx
	) noexcept -> CopyConst<void, Table>* {
		return static_cast<CopyConst<void, Table>*>(table.buffer()
			+ arch.offset(table.buffer_size(), comp_local_idx) + object_size * row_idx);
	}

	template<c_component Component, class Self>
	FR_FORCE_INLINE
	auto try_get_component_impl(
		this Self& self,
		EntityRecord record,
		const Arch& arch,
		CopyConst<EntityTable, Self>& table
	) noexcept -> CopyConst<Component, Self>* {
		const auto comp_id = ComponentTypeIdx::of<Component>;
		const auto comp_local_idx = arch.find_component_local_idx(comp_id);
		if (comp_local_idx.is_default())
			return nullptr;

		if constexpr (c_tag_component<Component>) {
			const auto& comp_info = self._component_infos.get_at_unchecked(comp_id);
			return static_cast<CopyConst<Component, Self>*>(comp_info.singleton_object());
		}
		else {
			const auto offset = arch.offset(table.buffer_size(), comp_local_idx.value());
			return std::launder(calc_component_ptr<CopyConst<Component, Self>>(table, offset,
				record.row_idx()));
		}
	}

	auto register_archetype_impl(
		ComponentTypeHash arch_hash,
		std::vector<ComponentTypeIdx>&& component_ids
	) -> ArchetypeIdx {
		const auto& new_arch = _archetypes.emplace_back(arch_hash, std::move(component_ids),
			_component_infos);
		const auto arch_idx = static_cast<ArchetypeIdx>(_archetypes.size() - 1zu);

		for (auto i = 0zu; i < new_arch.component_ids().size(); ++i) {
			auto& comp_info = _component_infos.get_at_unchecked(new_arch.component_ids()[i]);
			comp_info.link_archetype(arch_idx, i);
		}

		FR_LOG_TRACE("Registered archetype {}", arch_idx);
		return arch_idx;
	}

private:
	/// @details
	/// Record state            | arch_idx | table_idx | row_idx | version
	/// ------------------------------------------------------------------
	/// alive empty (e.g., new) | 0000     | xxxx      | xxxx    | xxxx
	/// alive with components   | xxxx     | xxxx      | xxxx    | xxxx
	/// dead                    | 1111     | xxxx      | xxxx    | xxxx
	class EntityRecord {
	public:
		static constexpr auto archetype_idx_bit_width = 32;
		static constexpr auto table_idx_bit_width = 32;
		static constexpr auto row_idx_bit_width = 20;
		static constexpr auto version_bit_width = Opts.eid_version_bits;
		static_assert(version_bit_width <= bit_width<typename Entity::RawType>);

		using ProxyType = typename Entity::RawType;
		static_assert(sizeof(ProxyType) >= sizeof(EntityTableIdx));
		static_assert(sizeof(ProxyType) >= sizeof(EntityTable::RowIdx));

		static constexpr auto empty_arch_idx = ProxyType{0};
		static constexpr auto dead_arch_idx = bitmask_low<ProxyType, archetype_idx_bit_width>;

		static constexpr auto max_archetype_idx = bitmask_low<ProxyType, archetype_idx_bit_width>
			- ProxyType{1};
		static constexpr auto max_table_idx = bitmask_low<ProxyType, table_idx_bit_width>
			- ProxyType{1};
		static constexpr auto max_table_size = bitmask_low<size_t, row_idx_bit_width>;
		static constexpr auto max_row_idx = bitmask_low<ProxyType, row_idx_bit_width>
			- ProxyType{1};

		// Make sure we can store every possible index within the largest table
		static_assert(max_table_size >= EntityTable::size_value(EntityTable::Size::Max));

		static FR_FORCE_INLINE constexpr
		auto make_empty(typename Entity::RawType version) noexcept -> EntityRecord {
			return {empty_arch_idx, 0, 0, version};
		}

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_CONVERSION
		constexpr
		EntityRecord(
			ArchetypeIdx arch_idx,
			EntityTableIdx table_idx,
			EntityTable::RowIdx row_idx,
			typename Entity::RawType version
		) noexcept:
			_arch_idx{arch_idx},
			_table_idx{table_idx},
			_row_idx{row_idx},
			_version{version}
		{ }
FR_DIAGNOSTIC_POP

		FR_FORCE_INLINE constexpr
		auto is_alive() const noexcept -> bool {
			return _arch_idx != dead_arch_idx;
		}

		FR_FORCE_INLINE constexpr
		auto is_dead() const noexcept -> bool {
			return _arch_idx == dead_arch_idx;
		}

		/// @brief Check whether entity has no components
		FR_FORCE_INLINE constexpr
		auto is_empty() const noexcept -> bool {
			return _arch_idx == empty_arch_idx;
		}

		FR_FORCE_INLINE constexpr
		void set_indices(
			ArchetypeIdx archetype_idx,
			EntityTableIdx table_idx,
			EntityTable::RowIdx row_idx
		) noexcept {
			FR_ASSUME(archetype_idx == dead_arch_idx || archetype_idx <= max_archetype_idx);
			FR_ASSUME(table_idx <= max_table_idx);
			FR_ASSUME(row_idx <= max_row_idx);
FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_CONVERSION
			_arch_idx = archetype_idx;
			_table_idx = table_idx;
			_row_idx = row_idx;
FR_DIAGNOSTIC_POP
		}

		FR_FORCE_INLINE constexpr
		void set_all(
			ArchetypeIdx archetype_idx,
			EntityTableIdx table_idx,
			EntityTable::RowIdx row_idx,
			typename Entity::RawType version
		) noexcept {
			FR_ASSUME(archetype_idx == dead_arch_idx || archetype_idx <= max_archetype_idx);
			FR_ASSUME(table_idx <= max_table_idx);
			FR_ASSUME(row_idx <= max_row_idx);
FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_CONVERSION
			_arch_idx = archetype_idx;
			_table_idx = table_idx;
			_row_idx = row_idx;
			_version = version;
FR_DIAGNOSTIC_POP
		}

		FR_FORCE_INLINE constexpr
		void set_empty() noexcept { set_indices(empty_arch_idx, _table_idx, _row_idx); }

		FR_FORCE_INLINE constexpr
		void set_dead() noexcept { set_indices(dead_arch_idx, _table_idx, _row_idx); }

		FR_FORCE_INLINE constexpr
		auto archetype_idx() const noexcept -> ArchetypeIdx { return _arch_idx; }

		FR_FORCE_INLINE constexpr
		auto table_idx() const noexcept -> EntityTableIdx { return _table_idx; }

		FR_FORCE_INLINE constexpr
		auto row_idx() const noexcept -> EntityTable::RowIdx { return _row_idx; }

		FR_FORCE_INLINE constexpr
		auto version() const noexcept -> typename Entity::RawType { return _version; }

	private:
		ProxyType _arch_idx: archetype_idx_bit_width;
		ProxyType _table_idx: table_idx_bit_width;
		ProxyType _row_idx: row_idx_bit_width;
		ProxyType _version: Opts.eid_version_bits;
	};

	ComponentInfoMap _component_infos;
	/// @brief Custom SparseSet-like data structure, but we only store the sparse part.
	/// The dense arrays are stored in the entity tables alongside the component data
	std::vector<EntityRecord> _entity_records;
	WithDefault<size_t, 0zu> _alive_entity_count;
	WithDefault<size_t, 0zu> _empty_entity_count;
	/// @note Append-only
	std::vector<Arch> _archetypes = {Arch{empty_init}};
	/// @note Append-only
	std::vector<EntityTable> _tables;
	WithDefaultValue<npos_for<EntityTableIdx>> _reserved_tables_heads[EntityTable::num_sizes];
	WithDefaultValue<npos_for<EntityTableIdx>> _reserved_tables_tails[EntityTable::num_sizes];
	WithDefaultValue<npos_for<EntityTableIdx>> _dead_tables_head;
	WithDefaultValue<npos_for<EntityTableIdx>> _dead_tables_tail;
	WithDefault<size_t, 0zu> _reserved_memory;
	WithDefault<size_t, 0zu> _reserved_memory_limit;
	/// @brief Entities that once have been allocated but now are destroyed
	/// @todo TODO: Make a ring buffer
	std::vector<Entity> _dead_entities;
};

} // namespace fr
#endif // include guard
