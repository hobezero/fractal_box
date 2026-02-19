#ifndef FRACTAL_BOX_RUNTIME_QUERIES_HPP
#define FRACTAL_BOX_RUNTIME_QUERIES_HPP

#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/control_flow.hpp"
#include "fractal_box/runtime/world_types.hpp"

namespace fr {

// Query definitions
// -----------------

// Query parameter types:
//
// Component    | RW access | R access   | No access
// -------------|-----------|------------|----------
// Required     | `T`       | `const T`  | `With<T>`
// Optional     | `T*`      | `const T*` | `Has<T>`
// Filtered out | none      | none       | `Without<T>`

// Query sink parameters:
//
// Query Parameter | `for_each` Parameter | `for_each_array` Parameter
// ----------------|----------------------|---------------------------
// `T`             | `T&`                 | `T*`
// `const T`       | `const T&`           | `const T*`
// `With<T>`       | none                 | none
//                 |                      |
// `T*`            | `T*`                 | `T*`
// `const T*`      | `const T*`           | `const T*`
// `Has<T>`        | `Has<T>`             | `Has<T>`
//                 |                      |
// `Without<T>`    | none                 | none

namespace detail {

enum class QueryParamKind {
	None,
	Required,
	Optional,
	FilteredOut,
};

// Required components
// """""""""""""""""""

template<class T>
struct QueryParamTraits {
	using Stripped = void;
	static constexpr auto kind = QueryParamKind::None;
	static constexpr auto access = Access::None;
	/// @brief Parameters that are actually passed to the callback (sink)
	static constexpr auto is_sinked = false;
	using ForEachParam = void;
	using ForEachArrayParam = void;
};

template<c_component T>
struct QueryParamTraits<T> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Required;
	static constexpr auto access = Access::ReadWrite;
	static constexpr auto is_sinked = true;
	using ForEachParam = T&;
	using ForEachArrayParam = T*;

	static FR_FORCE_INLINE constexpr
	auto make_for_each_array_param(Stripped* comp) noexcept -> ForEachArrayParam { return comp; }

	static FR_FORCE_INLINE constexpr
	auto make_for_each_param(ForEachArrayParam p, size_t i) noexcept -> ForEachParam {
		return p[i];
	}
};

template<c_component T>
struct QueryParamTraits<const T> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Required;
	static constexpr auto access = Access::ReadOnly;
	static constexpr auto is_sinked = true;
	using ForEachParam = const T&;
	using ForEachArrayParam = const T*;

	static FR_FORCE_INLINE constexpr
	auto make_for_each_array_param(Stripped* comp) noexcept -> ForEachArrayParam { return comp; }

	static FR_FORCE_INLINE constexpr
	auto make_for_each_param(ForEachArrayParam p, size_t i) noexcept -> ForEachParam {
		return p[i];
	}
};

template<class T>
struct QueryParamTraits<With<T>> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Required;
	static constexpr auto access = Access::None;
	static constexpr auto is_sinked = false;
	using ForEachParam = void;
	using ForEachArrayParam = void;
};

// Optional components
// """""""""""""""""""

template<c_component T>
struct QueryParamTraits<T*> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Optional;
	static constexpr auto access = Access::ReadWrite;
	static constexpr auto is_sinked = true;
	using ForEachParam = T*;
	using ForEachArrayParam = T*;

	static FR_FORCE_INLINE constexpr
	auto make_for_each_array_param(Stripped* p) noexcept -> ForEachArrayParam { return p; }

	static FR_FORCE_INLINE constexpr
	auto make_for_each_param(ForEachArrayParam p, size_t i) noexcept -> ForEachParam {
		return p ? std::addressof(p[i]) : nullptr;
	}
};

template<c_component T>
struct QueryParamTraits<const T*> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Optional;
	static constexpr auto access = Access::ReadOnly;
	static constexpr auto is_sinked = true;
	using ForEachParam = const T*;
	using ForEachArrayParam = const T*;

	static FR_FORCE_INLINE constexpr
	auto make_for_each_array_param(Stripped* p) noexcept -> ForEachArrayParam { return p; }

	static FR_FORCE_INLINE constexpr
	auto make_for_each_param(ForEachArrayParam p, size_t i) noexcept -> ForEachParam {
		return p ? std::addressof(p[i]) : nullptr;
	}
};

template<class T>
struct QueryParamTraits<Has<T>> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::Optional;
	static constexpr auto access = Access::None;
	static constexpr auto is_sinked = true;
	using ForEachParam = Has<T>;
	using ForEachArrayParam = Has<T>;

	static FR_FORCE_INLINE constexpr
	auto make_for_each_array_param(Stripped* comp) noexcept -> ForEachArrayParam {
		return Has<T>{comp};
	}

	static FR_FORCE_INLINE constexpr
	auto make_for_each_param(ForEachArrayParam p, size_t) noexcept -> ForEachParam {
		return p;
	}
};

// Filtered-out components
// """""""""""""""""""""""

template<class T>
struct QueryParamTraits<Without<T>> {
	using Stripped = T;
	static constexpr auto kind = QueryParamKind::FilteredOut;
	static constexpr auto access = Access::None;
	static constexpr auto is_sinked = false;
	using ForEachParam = void;
	using ForEachArrayParam = void;
};

template<class T>
using StripQueryParam = typename QueryParamTraits<T>::Stripped;

template<class T>
using IsAlgoQueryParam = BoolC<
	QueryParamTraits<T>::kind == QueryParamKind::Required
	|| QueryParamTraits<T>::kind == QueryParamKind::FilteredOut
>;

template<class T>
using IsRequiredQueryParam = BoolC<QueryParamTraits<T>::kind == QueryParamKind::Required>;

template<class T>
using IsOptionalQueryParam = BoolC<QueryParamTraits<T>::kind == QueryParamKind::Optional>;

template<class T>
using IsFilteredOutQueryParam = BoolC<QueryParamTraits<T>::kind == QueryParamKind::FilteredOut>;

template<class T>
using IsSinkedQueryParam = BoolC<QueryParamTraits<T>::is_sinked>;

template<class T>
using QueryForEachParam = typename QueryParamTraits<T>::ForEachParam;

template<class T>
using QueryForEachArrayParam = typename QueryParamTraits<T>::ForEachArrayParam;

template<class... Params>
using SinkedQueryParams = MpFilter<MpList<Params...>, IsSinkedQueryParam>;

template<class... Params>
using QueryForEachParams = MpTransform<SinkedQueryParams<Params...>, QueryForEachParam>;

template<class... Params>
using QueryForEachArrayParams = MpTransform<SinkedQueryParams<Params...>, QueryForEachArrayParam>;

/// @note Intentionally SFINAE-unfriendly
template<class... Params>
inline consteval
auto validate_query_params() {
	using ParamList = MpList<Params...>;
	using RequiredParams = MpFilter<ParamList, IsRequiredQueryParam>;
	using Stripped = MpTransform<ParamList, StripQueryParam>;

	if constexpr (sizeof...(Params) == 0)
		static_assert(false, "Query has no parameters");
	else if constexpr (mp_size<RequiredParams> == 0)
		static_assert(false, "Request at least one required component");
	else {
		for_each_type<Stripped>([]<class T> {
			static_assert(mp_count<Stripped, T> == 1, "Duplicate component");
		});
	}

	return true;
}

} // namespace detail

template<class T>
concept c_query_param = detail::QueryParamTraits<T>::kind != detail::QueryParamKind::None;

template<class Sink, class TWorld, class... Params>
concept c_query_for_each_array_sink
	= []<class... SinkParams>(MpList<SinkParams...>) {
		using Entity = typename TWorld::Entity;
		return requires(Sink& sink, size_t n, const Entity* eids, SinkParams... params) {
			{ sink(n, eids, params...) } -> c_void_or_control_flow;
		};
	}(detail::QueryForEachArrayParams<Params...>{})
	|| []<class... SinkParams>(MpList<SinkParams...>) {
		return requires(Sink& sink, size_t n, SinkParams... params) {
			{ sink(n, params...) } -> c_void_or_control_flow;
		};
	}(detail::QueryForEachArrayParams<Params...>{})
;

template<class Sink, class TWorld, class... Params>
concept c_query_for_each_sink
	= []<class... SinkParams>(MpList<SinkParams...>) {
		using Entity = typename TWorld::Entity;
		return requires(Sink& sink, Entity eid, SinkParams... params) {
			 { sink(eid, params...) } -> c_void_or_control_flow;
		};
	}(detail::QueryForEachParams<Params...>{})
	|| []<class... SinkParams>(MpList<SinkParams...>) {
		return requires(Sink& sink, SinkParams... params) {
			{ sink(params...) } -> c_void_or_control_flow;
		};
	}(detail::QueryForEachParams<Params...>{})
;

namespace detail {

class QueryLinkCursor {
public:
	explicit
	QueryLinkCursor() = default;

	explicit constexpr
	QueryLinkCursor(const CompArchLink* start, size_t size) noexcept:
		_ptr{start},
		_end{start + size}
	{ }

	FR_FORCE_INLINE
	auto size() const noexcept -> size_t { return static_cast<size_t>(_end - _ptr); }

	explicit FR_FORCE_INLINE
	operator bool() const noexcept { return _ptr != _end; }

	FR_FORCE_INLINE
	void operator++() noexcept {
		FR_ASSERT_AUDIT(_ptr && _ptr != _end);
		++_ptr;
	}

	FR_FORCE_INLINE
	auto operator*() const -> const CompArchLink& {
		FR_ASSERT_AUDIT(_ptr && _ptr != _end);
		return *_ptr;
	}

	FR_FORCE_INLINE
	auto operator->() const -> const CompArchLink* {
		FR_ASSERT_AUDIT(_ptr && _ptr != _end);
		return _ptr;
	}

private:
	const CompArchLink* _ptr = nullptr;
	const CompArchLink* _end = nullptr;
};

} // namespace detail

/// @brief ECS query
/// @todo
///   TODO: Support complex queries: `And`, `Or`, `Not`, ...
///   TODO: Support quering of entities with no components
template<class TWorld, c_query_param... Params>
class UncachedQuery {
	using Entity = typename TWorld::Entity;
	using Archetype = typename TWorld::Arch;
	static_assert(detail::validate_query_params<Params...>());

public:
	/// @todo TODO: Replace concept requirement with custom static_assert
	template<c_query_for_each_array_sink<TWorld, Params...> Sink>
	auto for_each_array(Sink&& sink) const -> ControlFlow {
		// # Algorithm:
		// Filter all requests to only required params (`T`, `const T`, `With<T>`) and
		// filtered-out params (`Without<T`>). Required params come first. Store compile-time
		// mapping to the index in the original parameter list.
		// Find required component with the smallest number of archetype links ("spine").
		// Initialize cursors for all the sets.
		// For each element `x` of the spine:
		//     Advance all other cursors untill we meet an element >= `x`.
		//     If all required cursors have met `x` exactly, and no filtered-out cursors have met
		//     `x`:
		//        Check optional params (`T*`, `const T*`, `Has<T>`)
		//        Call sink on `x` archetype, passing all sinked params
		//     If any of the required cursors have reached the end of their set:
		//        Return
		// Time complexity: O(n), where n = total number of elements in all of the sets/total number
		// of archetype links
		// TODO: Look into proper algorithms: https://arxiv.org/pdf/1103.2409
		using namespace detail;

		const ComponentInfo* comp_infos[sizeof...(Params)] = {
			_world->_component_infos.template try_get<StripQueryParam<Params>>()...
		};

#if 0
		FR_LOG_DEBUG_MSG("Links: ");
		for (const auto* comp_info : comp_infos) {
			static constexpr auto by_arch_idx = [](const auto& x) { return x.arch_idx(); };
			FR_LOG_DEBUG("{}: {}", comp_info->type_info().name(),
				fmt::join(std::views::transform(comp_info->archetype_links(), by_arch_idx), ", ")) ;
		}
#endif

		// Find the spine
		QueryLinkCursor required_cursors[std::max(mp_size<RequiredParams>, 1zu)];
		auto spine_idx = 0zu;
		auto should_exit_early = mp_size<RequiredParams> == 0zu;
		unroll<mp_size<RequiredParams>>([&]<size_t ReqIdx> FR_FORCE_INLINE_L {
			if (should_exit_early)
				return;
			constexpr auto all_idx = MpAt<RequiredParams, ReqIdx>::index;
			if (!comp_infos[all_idx] || comp_infos[all_idx]->archetype_links().empty()) {
				should_exit_early = true; // No archetypes = cant't match anything
				return;
			}
			const auto& links = comp_infos[all_idx]->archetype_links();
			required_cursors[ReqIdx] = QueryLinkCursor{links.data(), links.size()};
			if (ReqIdx == 0 || links.size() < required_cursors[spine_idx].size())
				spine_idx = ReqIdx;
		});
		if (should_exit_early)
			return ControlFlow::Continue;

		// Move the spine to the start
		std::swap(required_cursors[0], required_cursors[spine_idx]);

		// Initialize filtered-out cursors
		QueryLinkCursor filtered_out_cursors[std::max(mp_size<FilteredOutParams>, 1zu)];
		auto filtered_out_count = 0zu;
		unroll<mp_size<FilteredOutParams>>([&]<size_t FiltIdx> FR_FORCE_INLINE_L mutable {
			constexpr auto all_idx = MpAt<FilteredOutParams, FiltIdx>::index;
			if (!comp_infos[all_idx] || comp_infos[all_idx]->archetype_links().empty())
				return;
			const auto& links = comp_infos[all_idx]->archetype_links();
			filtered_out_cursors[filtered_out_count] = QueryLinkCursor{links.data(), links.size()};
			++filtered_out_count;
		});

		// Match every element of the spine
		for (auto& spine = required_cursors[0]; spine; ++spine) {
			auto spine_arch_idx = spine->arch_idx();
			auto intersected_count = 0zu;
			for (auto i = 1zu; i < mp_size<RequiredParams>; ++i) {
				for (auto& cursor = required_cursors[i]; cursor; ++cursor) {
					if (cursor->arch_idx() == spine_arch_idx) {
						++intersected_count;
						break;
					}
					if (cursor->arch_idx() > spine_arch_idx)
						break;
				}
			}
			auto is_filtered_out = false;
			for (auto i = 0zu; i < filtered_out_count; ++i) {
				for (auto& cursor = filtered_out_cursors[i]; cursor; ++cursor) {
					if (cursor->arch_idx() == spine_arch_idx) {
						is_filtered_out = true;
						break;
					}
					if (cursor->arch_idx() > spine_arch_idx)
						break;
				}
			}

			if (!is_filtered_out && intersected_count == mp_size<RequiredParams> - 1zu) {
				size_t comp_local_idxs[mp_size<RequiredParams>];
				for (auto i = 0zu; i < mp_size<RequiredParams>; ++i)
					comp_local_idxs[i] = required_cursors[i]->comp_local_idx();
				// Restore original order
				std::swap(comp_local_idxs[0], comp_local_idxs[spine_idx]);
#if 0
				FR_LOG_DEBUG("Intersected: {}", spine_arch_idx);
#endif
				auto r = call_sink(sink, spine_arch_idx, comp_local_idxs);
				if (r.is_break())
					break;
				if (r.is_return())
					return r;
			}
		}

		return ControlFlow::Continue;
	}

	/// @todo TODO: Replace concept requirement with custom static_assert
	template<c_query_for_each_sink<TWorld, Params...> Sink>
	auto for_each(Sink&& sink) const -> ControlFlow {
		using namespace detail;
		return for_each_array([&sink](
			size_t n, const Entity* eids, auto&&... params
		) FR_FORCE_INLINE_L {
			[&]<class... SinkedParams>(MpList<SinkedParams...>) FR_FORCE_INLINE_L {
				for (auto i = 0zu; i < n; ++i) {
					// TODO: Hoist optional component check out of the loop
					if constexpr (requires {
						sink(eids[i],
							QueryParamTraits<SinkedParams>::make_for_each_param(params, i)...);
					}) {
						const auto r = cflow_invoke(sink, eids[i],
							QueryParamTraits<SinkedParams>::make_for_each_param(params, i)...);
						if (r.is_break_or_return())
							return r;
					}
					else if constexpr (requires {
						sink(QueryParamTraits<SinkedParams>::make_for_each_param(params, i)...);
					}) {
						const auto r = cflow_invoke(sink,
							QueryParamTraits<SinkedParams>::make_for_each_param(params, i)...);
						if (r.is_break_or_return())
							return r;
					}
					else
						static_assert(false, "Sink callback takes incorrect arguments");
				}
				return ControlFlow::Continue;
			}(detail::SinkedQueryParams<Params...>{});
		});
	}

	auto count() const noexcept -> size_t {
		auto count = 0zu;
		for_each_array([&](size_t n, auto&&...) FR_FORCE_INLINE_L { count += n; });
		return count;
	}

private:
	friend TWorld;

	using ParamList = MpEnumerate<MpList<Params...>>;
	using RequiredParams = MpFilterProj<ParamList, detail::IsRequiredQueryParam, MpSecond>;
	using FilteredOutParams = MpFilterProj<ParamList, detail::IsFilteredOutQueryParam, MpSecond>;
	using SinkedParams = MpFilterProj<ParamList, detail::IsSinkedQueryParam, MpSecond>;
	using CompLocalIdxsArr = size_t[mp_size<RequiredParams>];

	explicit constexpr
	UncachedQuery(TWorld& world) noexcept: _world{&world} { }

	template<class Sink, class... Ts>
	FR_FORCE_INLINE
	auto sink_table(
		Sink&& sink, size_t num, const Entity* eids, Ts&&... params
	) const -> ControlFlow {
		if constexpr (requires { sink(num, eids, std::forward<Ts>(params)...); }) {
			const auto r = cflow_invoke(sink, num, eids, std::forward<Ts>(params)...);
			if (r.is_break_or_return())
				return r;
		}
		else if constexpr (requires { sink(num, std::forward<Ts>(params)...); }) {
			const auto r = cflow_invoke(sink, num, std::forward<Ts>(params)...);
			if (r.is_break_or_return())
				return r;
		}
		else
			static_assert(false, "Sink callback takes incorrect arguments");
		return {};
	}

	template<class T>
	FR_FORCE_INLINE
	auto make_sink_param(
		const Archetype& arch, EntityTable& table, const CompLocalIdxsArr& required_local_idxs
	) const noexcept {
		using Param = typename T::Type;
		using Traits = detail::QueryParamTraits<Param>;
		using Component = typename Traits::Stripped;
		const auto get_ptr = [&](size_t local_idx) FR_FORCE_INLINE_L {
			if constexpr (c_tag_component<Component>) {
				return static_cast<Component*>(_world->template get_tag_component<Component>());
			}
			else {
				return _world->template calc_component_ptr<Component>(table,
					arch.offset(table.buffer_size(), local_idx), 0);
			}
		};
		if constexpr (Traits::kind == detail::QueryParamKind::Required) {
			static constexpr auto req_idx = mp_find<RequiredParams, T>;
			return get_ptr(required_local_idxs[req_idx]);
		}
		else if constexpr (Traits::kind == detail::QueryParamKind::Optional) {
			// TODO: Find local idx once per archetype, not once per table
			const auto local_idx = arch.find_component_local_idx(ComponentTypeIdx::of<Component>);
			if constexpr (Traits::access == Access::None) {
				return Param{!local_idx.is_default()};
			}
			else {
				auto* const ptr = local_idx.is_default()
					? nullptr
					: get_ptr(local_idx.value());
				return Traits::make_for_each_array_param(ptr);
			}
		}
		else {
			static_assert(false);
		}
	}

	template<class Sink>
	FR_FORCE_INLINE
	auto call_sink(
		Sink&& sink, ArchetypeIdx arch_idx, const CompLocalIdxsArr& required_local_idxs
	) const -> ControlFlow {
		const auto& arch = _world->_archetypes[arch_idx];
		return [&]<class... Ts>(MpList<Ts...>) FR_FORCE_INLINE_L -> ControlFlow {
			for (auto table_idx : arch.table_idxs()) {
				auto& table = _world->_tables[table_idx];
				if (table.is_empty())
					continue;
				auto r = sink_table(
					sink,
					table.alive_count(),
					&_world->get_lookback_id_impl(table, 0),
					make_sink_param<Ts>(arch, table, required_local_idxs)...
				);
				if (r.is_break_or_return())
					return r;

			}
			return {};
		}(SinkedParams{});
	}

private:
	TWorld* _world;
};

template<class T>
inline constexpr auto is_uncached_query = false;

template<class TWorld, class... Params>
inline constexpr auto is_uncached_query<UncachedQuery<TWorld, Params...>> = true;

template<class T>
concept c_uncached_query = is_uncached_query<T>;

} // namespace fr
#endif // include guard
