#ifndef FRACTAL_BOX_RUNTIME_PROFILER_HPP
#define FRACTAL_BOX_RUNTIME_PROFILER_HPP

/// @see https://github.com/DarkWanderer/metrics-cpp
/// @see https://vittorioromeo.com/index/blog/sfex_profiler.html

#include <span>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/chrono_types.hpp"
#include "fractal_box/core/containers/linear_flat_set.hpp"
#include "fractal_box/core/containers/unordered_map.hpp"
#include "fractal_box/core/hashing/hashed_string.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/iterator_utils.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/timeline.hpp"
#include "fractal_box/math/math.hpp"

namespace fr {

enum class TraceEventKind: uint8_t {
	StackPush,
	StackPop,
#if 0
	SegmentStart,
	SegmentEnd,
#endif
	Point,
};

enum class TraceEventSemantics: uint8_t {
	Generic,
	SameAsPush,
	Tick,
	Frame,
	Phase, // Index is PhaseTypeIdx
	System, // Index is SystemTypeIdx
	Function,
	Section,
};

struct TraceEventUserOpts {
	using Index = uint32_t;

public:
	TraceEventSemantics semantics = TraceEventSemantics::Generic;
	Index index = 0u;
	bool has_extras = false;
};

class TraceEventUserData {
public:
	using Index = TraceEventUserOpts::Index;

	static constexpr auto semantics_num_bits = 3u;
	static constexpr auto index_num_bits
		= static_cast<unsigned>(sizeof(Index)) * CHAR_BIT - semantics_num_bits - 1u;

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_CONVERSION
	explicit(false) constexpr
	TraceEventUserData(TraceEventUserOpts opts = {}) noexcept:
		_semantics{opts.semantics},
		_index{opts.index},
		_has_extras{opts.has_extras}
	{
		FR_PANIC_CHECK(opts.index < (Index{1} << index_num_bits));
	}
FR_DIAGNOSTIC_POP

	constexpr
	auto semantics() const noexcept -> TraceEventSemantics { return _semantics; }

	constexpr
	auto has_extras() const noexcept -> bool { return _has_extras; }

	constexpr
	void set_has_extras(bool value) noexcept { _has_extras = value; }

	constexpr
	auto index() const noexcept -> Index { return _index; }

private:
	TraceEventSemantics _semantics: semantics_num_bits;
	Index _index: index_num_bits;
	bool _has_extras: 1u;
};

class TraceEvent {
public:
FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_DISABLE_CONVERSION
	constexpr
	TraceEvent(
		TraceEventKind kind,
		StringId label_id,
		RawDuration time_since_start,
		TraceEventUserData user_data
	) noexcept:
		_kind{kind},
		_time{time_since_start.count()},
		_label_id{label_id},
		_user_data{user_data}
	{
		FR_PANIC_CHECK(time_since_start.count() < (TimeData{1} << time_data_num_bits));
	}
FR_DIAGNOSTIC_POP

	constexpr
	auto kind() const noexcept -> TraceEventKind { return _kind; }

	constexpr
	auto label_id() const noexcept -> StringId { return _label_id; }

	constexpr
	auto user_data() const noexcept -> TraceEventUserData { return _user_data; }

	constexpr
	auto time_since_start() const noexcept -> RawDuration { return RawDuration{_time}; }

private:
	using TimeData = RawDuration::rep;
	static constexpr auto kind_num_bits = 3u;
	static constexpr auto time_data_num_bits
		= static_cast<unsigned>(sizeof(TimeData)) * CHAR_BIT - kind_num_bits;

private:
	TraceEventKind _kind: kind_num_bits;
	/// @note 2**(64 - 3 - 1) ns = 5.76e17 ns = 18.27 years
	TimeData _time: time_data_num_bits;
	StringId _label_id;
	TraceEventUserData _user_data;
};

class Tracer;

class TraceStackGuard {
public:
	constexpr
	TraceStackGuard(Tracer* tracer, StringId label_id) noexcept:
		_tracer{tracer},
		_label_id{label_id}
	{ }

	TraceStackGuard(const TraceStackGuard&) = delete;
	auto operator=(const TraceStackGuard&) -> TraceStackGuard& = delete;

	constexpr
	TraceStackGuard(TraceStackGuard&& other) noexcept:
		_tracer{std::exchange(other._tracer, nullptr)},
		_label_id{other._label_id}
	{ }

	constexpr
	auto operator=(TraceStackGuard&&) noexcept = delete;

	constexpr
	~TraceStackGuard();

	constexpr
	auto release() noexcept { _tracer = nullptr; }

	constexpr
	void execute();

private:
	Tracer* _tracer;
	StringId _label_id;
};

/// @todo TODO: Consider mutlithreading
class Tracer {
public:
	using Clock = SteadyClock;
	static constexpr auto tick_label = HashedStrView{"TracerTick"};

	void start() {
		FR_PANIC_CHECK(_start_tpoint == SteadyTimePoint{});
		register_label(tick_label);
		_start_tpoint = Clock::now();
	}

	auto register_label(StringId id, std::string label) -> bool {
		return _labels.emplace(id, std::move(label)).second;
	}

	auto register_label(StringId id, std::string_view label) -> bool {
		return _labels.emplace(id, std::string(label)).second;
	}

	auto register_label(HashedStr hashed_label) -> bool {
		const auto id = StringId{hashed_label};
		return _labels.emplace(id, hashed_label.release()).second;
	}

	auto register_label(HashedStrView hashed_label) -> bool {
		return _labels.emplace(hashed_label, std::string(hashed_label.str())).second;
	}

	auto trace_stack_push(StringId label_id, TraceEventUserData user_data = {}) -> TraceStackGuard {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		FR_ASSERT(_labels.contains(label_id));

		_events.emplace_back(TraceEventKind::StackPush, label_id, since_start(), user_data);
		_label_stack.push_back(label_id);

		return {this, label_id};
	}

	void trace_stack_pop(StringId label_id) {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		FR_PANIC_CHECK(_label_stack.back() == label_id);

		_events.emplace_back(TraceEventKind::StackPop, label_id, since_start(),
			TraceEventUserOpts{.semantics = TraceEventSemantics::SameAsPush});
		_label_stack.pop_back();
	}

#if 0
	auto trace_segment_start(
		StringId label_id, TraceEventUserData user_data = {}
	) {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		FR_ASSERT(_labels.contains(label_id));

		_events.emplace_back(TraceEventKind::SegmentStart, label_id, since_start(), user_data);

		return ScopeExit{[this, label_id] {
			trace_segment_end(label_id);
		});
	}

	void trace_segment_end(StringId label_id) {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		FR_ASSERT(_labels.contains(label_id));

		_events.emplace_back(TraceEventKind::SegmentEnd, label_id, since_start(),
			TraceEventUserData{TraceEventSemantics::CopyStart});
	}
#endif

	void trace_point(
		StringId label_id, TraceEventUserData user_data = {}
	) {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		FR_ASSERT(_labels.contains(label_id));

		_events.emplace_back(TraceEventKind::Point, label_id, since_start(), user_data);
	}

	void trace_tick() noexcept {
		FR_PANIC_CHECK(_start_tpoint != SteadyTimePoint{});
		trace_point(tick_label, TraceEventUserOpts{.semantics = TraceEventSemantics::Tick});
	}

	void consume_cold_events() {
		const auto pending = cold_events();
		const auto start_idx = ptrdiff_t{pending.data() - _events.data()};
		const auto end_idx = start_idx + static_cast<ptrdiff_t>(pending.size());
		_events.erase(_events.begin() + start_idx, _events.begin() + end_idx);
	}

	auto events() const noexcept -> std::span<const TraceEvent> { return _events; }

	/// @brief Find all events untill the last tick
	auto cold_events() const noexcept -> std::span<const TraceEvent> {
		for (auto i = _events.size(); i-- != 0;) {
			if (_events[i].user_data().semantics() == TraceEventSemantics::Tick) {
				// NOTE: Tick event is not included in the range, there is no off-by-one error
				return {_events.begin(), _events.begin() + static_cast<ptrdiff_t>(i)};
			}
		}
		return {};
	}

	auto label_from_id(StringId label_id) const noexcept -> const std::string& {
		const auto it = _labels.find(label_id);
		FR_ASSERT(it != _labels.end());
		return it->second;
	}

private:
	FR_FORCE_INLINE
	auto since_start() const -> RawDuration { return Clock::now() - _start_tpoint; }

private:
	UnorderedMap<StringId, std::string> _labels;
	SteadyTimePoint _start_tpoint;
	std::vector<TraceEvent> _events;
	std::vector<StringId> _label_stack;
};

struct ProfilerConfig {
	static
	auto make() noexcept -> ProfilerConfig;

public:
	struct RefreshConfig {
		RawDuration refresh_period;
		uint32_t min_batch_size;
		uint32_t max_batch_size;
	};

	RefreshConfig simple_refresh;
	RefreshConfig extra_refresh;
};

struct TreeWalkControl {
	bool recurse = true;
};

/// @todo TODO Look into EasyProfiler design (https://github.com/yse/easy_profiler)
class Profiler {
public:
	using TickIdType = TickId<Profiler>;

	struct SimpleStats {
		FDuration min = FDuration{f_nan};
		FDuration max = FDuration{f_nan};
		FDuration avg = FDuration{f_nan};
		float samples_per_tick = f_nan;
	};

	class SimpleStatsCollector {
	public:
		constexpr
		void add(RawDuration sample) noexcept;

		constexpr
		auto should_refresh(
			const ProfilerConfig& prof_config, SteadyTimePoint now
		) const noexcept -> bool;

		[[nodiscard]] constexpr
		auto refresh(SteadyTimePoint now, TickIdType curr_tick) -> SimpleStats;

		constexpr
		auto last_refresh_tick() const noexcept -> TickIdType { return _last_refresh_tick; }

		constexpr
		auto batch_size() const noexcept -> size_t { return _sample_count; }

	private:
		SteadyTimePoint _last_refresh;
		TickIdType _last_refresh_tick;
		RawDuration _accum = RawDuration::zero();
		uint32_t _sample_count = 0u;
		FDuration _min = FDuration{f_nan};
		FDuration _max = FDuration{f_nan};
	};

	struct ExtraStats {
		FDuration p50 = FDuration{f_nan};
		FDuration p95 = FDuration{f_nan};
	};

	class ExtraStatsCollector {
	public:
		constexpr
		void add(RawDuration sample);

		constexpr
		auto should_refresh(
			const ProfilerConfig& prof_config, SteadyTimePoint now
		) const noexcept -> bool;

		[[nodiscard]] constexpr
		auto refresh(SteadyTimePoint now) -> ExtraStats;

		constexpr
		auto batch_size() const noexcept -> size_t { return _batch.size(); }

	private:
		SteadyTimePoint _last_refresh;
		std::vector<FDuration> _batch;
	};

	struct Node {
		Node(std::string new_name, TraceEventUserData new_user_data):
			name(std::move(new_name)),
			user_data{new_user_data}
		{ }

		Node(const Node&) = delete;
		auto operator=(const Node&) -> Node& = delete;

		Node(Node&&) = delete;
		auto operator=(Node&&) -> Node& = default;

		~Node() = default;

	public:
		std::string name;
		StringId parent;
		std::vector<StringId> children;
		SimpleStatsCollector collector;
		SimpleStats stats;
		FDuration last_sample;
		TraceEventUserData user_data;
	};

	struct ExtraNode {
		ExtraStats stats;
		ExtraStatsCollector collector;
	};

	void update(
		const Tracer& tracer,
		const ProfilerConfig& config,
		SteadyTimePoint now,
		bool force_refresh = false
	);

	auto nodes() const noexcept -> const UnorderedMap<StringId, Node>& { return _forest; }

	auto extra_nodes() const noexcept -> const UnorderedMap<StringId, ExtraNode>& {
		return _extra_nodes;
	}

	void add_extra_node(StringId node_id) {
		FR_LOG_DEBUG("Adding extra node '{}'", node_id.hash());
		const auto node_it = _forest.find(node_id);
		FR_PANIC_CHECK(node_it != _forest.end());
		node_it->second.user_data.set_has_extras(true);
		_extra_nodes.try_emplace(node_id);
	}

	void remove_extra_node(StringId node_id) {
		FR_LOG_DEBUG("Removing extra node '{}'", node_id.hash());
		const auto node_it = _forest.find(node_id);
		FR_PANIC_CHECK(node_it != _forest.end());
		node_it->second.user_data.set_has_extras(false);
		_extra_nodes.erase(node_id);
	}

	auto node_roots() const noexcept -> const LinearFlatSet<StringId>& { return _roots; }

	template<class OutIt>
	void debug_print(OutIt out) const {
		const auto print_node = [out](
			StringId, const Node& node, const ExtraNode* extra_node, int depth
		) -> TreeWalkControl {
			fmt::format_to(out, "{:{}}{} | {:>6.3f} | {:>6.3f} {:>6.3f} {:>6.3f}",
				std::string_view{}, 3 * depth,
				node.name,
				chrono_cast<FDurationMs>(node.last_sample).count(),
				chrono_cast<FDurationMs>(node.stats.min).count(),
				chrono_cast<FDurationMs>(node.stats.avg).count(),
				chrono_cast<FDurationMs>(node.stats.max).count());
			if (extra_node) {
				fmt::format_to(out, " | {:>6.3f} {:>6.3f}",
					chrono_cast<FDurationMs>(extra_node->stats.p50).count(),
					chrono_cast<FDurationMs>(extra_node->stats.p95).count());
			}
			fmt::format_to(out, "\n");

			return {.recurse = true};
		};
		walk_tree_preorder(print_node);
	}

	template<class PreCallback, class PostCallback = NoOp>
	void walk_tree_preorder(
		PreCallback&& pre_fn,
		StringId root = {},
		PostCallback&& post_fn = {}
	) const {
		if (root) {
			walk_tree_preorder_impl(std::forward<PreCallback>(pre_fn),
				std::forward<PostCallback>(post_fn), root, 0);
		}
		else {
			for (auto root_id : _roots) {
				walk_tree_preorder_impl(std::forward<PreCallback>(pre_fn),
					std::forward<PostCallback>(post_fn), root_id, 0);
			}
		}
	}

	template<class PreCallback, class PostCallback>
	void walk_tree_stateful(
		PreCallback&& pre_fn,
		PostCallback&& post_fn,
		StringId root = {}
	) const {
		if (root) {
			walk_tree_stateful_impl(std::forward<PreCallback>(pre_fn),
				std::forward<PostCallback>(post_fn), root, 0);
		}
		else {
			for (auto root_id : _roots) {
				walk_tree_stateful_impl(std::forward<PreCallback>(pre_fn),
					std::forward<PostCallback>(post_fn), root_id, 0);
			}
		}
	}

private:
	struct Impl;

	template<class PreCallback, class PostCallback = NoOp>
	void walk_tree_preorder_impl(
		PreCallback&& pre_fn,
		PostCallback&& post_fn,
		StringId node_id,
		int depth
	) const {
		const auto& [id, node] = assert_iter(_forest, _forest.find(node_id));
		const auto extra_it = _extra_nodes.find(id);
		const auto* extra_node = extra_it == _extra_nodes.end() ? nullptr : &extra_it->second;

		const auto control = std::forward<PreCallback>(pre_fn)(id, node, extra_node, depth);
		if (control.recurse) {
			for (auto child_id : node.children) {
				walk_tree_preorder_impl(std::forward<PreCallback>(pre_fn),
					std::forward<PostCallback>(post_fn), child_id, depth + 1);
			}
			std::forward<PostCallback>(post_fn)(id, node, extra_node, depth);
		}
	}

	template<class PreCallback, class PostCallback = NoOp>
	void walk_tree_stateful_impl(
		PreCallback&& pre_fn,
		PostCallback&& post_fn,
		StringId node_id,
		int depth
	) const {
		const auto& [id, node] = assert_iter(_forest, _forest.find(node_id));
		const auto extra_it = _extra_nodes.find(id);
		const auto* extra_node = extra_it == _extra_nodes.end() ? nullptr : &extra_it->second;

		auto&& [state, control] = std::forward<PreCallback>(pre_fn)(id, node, extra_node, depth);
		if (control.recurse) {
			for (auto child_id : node.children) {
				walk_tree_stateful_impl(std::forward<PreCallback>(pre_fn),
					std::forward<PostCallback>(post_fn), child_id, depth + 1);
			}
			std::forward<PostCallback>(post_fn)(state, id, node, extra_node, depth);
		}
	}

private:
	LinearFlatSet<StringId> _roots;
	UnorderedMap<StringId, Node> _forest;
	/// @note All nodes have `SimpleStats`. Extra nodes are a kind of nodes that also have
	/// `ExtraStats`
	UnorderedMap<StringId, ExtraNode> _extra_nodes;
	TickIdType _tick_id;
};

inline constexpr
TraceStackGuard::~TraceStackGuard() { execute(); }

inline constexpr
void TraceStackGuard::execute() {
	if (_tracer) {
		_tracer->trace_stack_pop(_label_id);
		_tracer = nullptr;
	}
}

} // namespace fr
#endif
