#include "fractal_box/runtime/tracer_profiler.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "fractal_box/core/iterator_utils.hpp"
#include "fractal_box/core/logging.hpp"

namespace fr {

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS

auto ProfilerConfig::make() noexcept -> ProfilerConfig {
	using namespace std::chrono_literals;

	return {
		.simple_refresh = {
			.refresh_period = 1'000ms,
			.min_batch_size = 20,
			.max_batch_size = 5'000,
		},
		.extra_refresh = {
			.refresh_period = 1'500ms,
			.min_batch_size = 20,
			.max_batch_size = 2'000,
		}
	};
}

FR_DIAGNOSTIC_POP

constexpr
void Profiler::SimpleStatsCollector::add(RawDuration sample) noexcept {
	const auto fsample = chrono_cast<FDuration>(sample);
	_min = std::min(_min, fsample);
	_max = std::max(_max, fsample);
	_accum += sample;
	++_sample_count;
}

constexpr
auto Profiler::SimpleStatsCollector::should_refresh(
	const ProfilerConfig& prof_config, SteadyTimePoint now
) const noexcept -> bool {
	const auto& cfg = prof_config.simple_refresh;
	return batch_size() > cfg.max_batch_size
		|| (now - _last_refresh >= cfg.refresh_period && batch_size() >= cfg.min_batch_size);
}

constexpr
auto Profiler::SimpleStatsCollector::refresh(
	SteadyTimePoint now, TickIdType curr_tick
) -> SimpleStats {
	const auto has_samples = _sample_count > 0u;
	const auto num_ticks = static_cast<decltype(_sample_count)>(curr_tick - _last_refresh_tick);
	const auto stats = SimpleStats{
		.min = has_samples ? _min : FDuration{f_nan},
		.max = has_samples ? _max : FDuration{f_nan},
		.avg = has_samples ? chrono_cast<FDuration>(_accum) / _sample_count : FDuration{f_nan},
		.samples_per_tick = f_ratio_value(_sample_count, num_ticks)
	};

	_last_refresh = now;
	_last_refresh_tick = curr_tick;
	_sample_count = 0;
	_accum = RawDuration::zero();
	_min = FDuration{f_pos_inf};
	_max = FDuration{f_neg_inf};

	return stats;
}

constexpr
void Profiler::ExtraStatsCollector::add(RawDuration sample) {
	_batch.push_back(chrono_cast<FDuration>(sample));
}

constexpr
auto Profiler::ExtraStatsCollector::should_refresh(
	const ProfilerConfig& prof_config, SteadyTimePoint now
) const noexcept -> bool {
	const auto& cfg = prof_config.extra_refresh;
	return batch_size() > cfg.max_batch_size
		|| (now - _last_refresh >= cfg.refresh_period && batch_size() >= cfg.min_batch_size);
}

constexpr
auto Profiler::ExtraStatsCollector::refresh(SteadyTimePoint now) -> ExtraStats {
	auto stats = ExtraStats{
		.p50 = FDuration{f_nan},
		.p95 = FDuration{f_nan},
	};

	if (!_batch.empty()) {
		const auto p50_it = _batch.begin() + static_cast<ptrdiff_t>(_batch.size() / 2u);
		std::ranges::nth_element(_batch.begin(), p50_it, _batch.end(), std::greater<>{});

		const auto p95_it = _batch.begin() + static_cast<ptrdiff_t>(_batch.size() / 20u);
		std::ranges::nth_element(_batch.begin(), p95_it, p50_it, std::greater<>{});

		stats = {
			.p50 = *p50_it,
			.p95 = *p95_it,
		};
	}
	_last_refresh = now;
	_batch.clear();

	return stats;
}

struct Profiler::Impl {
	static
	auto handle_stack_push(Profiler& self, const Tracer& tracer, TraceEvent push_event) {
		const auto node_id = push_event.label_id();
		if (auto node_it = self._forest.find(node_id); node_it == self._forest.end()) {
			const auto& label = tracer.label_from_id(node_id);
			[[maybe_unused]] bool emplaced_;
			std::tie(node_it, emplaced_) = self._forest.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(node_id),
				std::forward_as_tuple(label, push_event.user_data())
			);
			self._roots.insert(node_id);
			FR_LOG_DEBUG("Profiler: added node '{}'", node_it->second.name);
		}
	}

	static
	auto handle_stack_pop(
		Profiler& self,
		const std::vector<TraceEvent>& stack,
		TraceEvent push_event,
		TraceEvent pop_event
	) {
		const auto node_id = pop_event.label_id();
		auto& node = assert_iter(self._forest, self._forest.find(node_id)).second;

		const auto [new_parent_id, new_parent_node] = [&] -> std::pair<StringId, Node*> {
			if (stack.empty())
				return {StringId{}, nullptr};
			auto& parent = assert_iter(self._forest, self._forest.find(stack.back().label_id()));
			return {parent.first, &parent.second};
		}();

		// Reparent if necessary
		if (node.parent != new_parent_id) {
			auto old_parent_name = std::string_view("null");
			if (node.parent) {
				auto&& [old_parent_id, old_parent_node] = assert_iter(self._forest,
					self._forest.find(node.parent));
				std::erase(old_parent_node.children, node_id);
				old_parent_name = old_parent_node.name;
			}
			else {
				self._roots.erase(node_id);
			}
			if (new_parent_node)
				new_parent_node->children.push_back(node_id);
			else
				self._roots.insert(node_id);
			FR_LOG_DEBUG("Profiler: node '{}' reparented from '{}' to '{}'", node.name,
				old_parent_name, new_parent_node ? std::string_view(new_parent_node->name)
					: std::string_view{});
			node.parent = new_parent_id;
		}
		// TODO: Change order within parent's children if necessary

		// Add sample
		const auto last_sample = pop_event.time_since_start() - push_event.time_since_start();
		node.last_sample = chrono_cast<FDuration>(last_sample);
		node.collector.add(last_sample);

		// TODO: What if user disabled extra stats in the UI but they are enabled in the code?
		// For now, we ignore the UI option
		const auto extra_it = self._extra_nodes.find(node_id);
		auto* const extra_node
			= extra_it != self._extra_nodes.end() ? &extra_it->second
			: push_event.user_data().has_extras() ? &self._extra_nodes[node_id]
			: nullptr;
		if (extra_node)
			extra_node->collector.add(last_sample);
	}

	template<bool IsForced>
	static
	auto refresh(Profiler& self, const ProfilerConfig& config, SteadyTimePoint now) {
		for (auto&& [id, node] : self._forest) {
			if constexpr (IsForced) {
				// TODO: Merge stats
			}
			else {
				if (node.collector.should_refresh(config, now))
					node.stats = node.collector.refresh(now, self._tick_id);
			}
		}
		for (auto&& [id, extra_node] : self._extra_nodes) {
			if constexpr (IsForced) {
				// TODO: Merge stats
			}
			else {
				if (extra_node.collector.should_refresh(config, now))
					extra_node.stats = extra_node.collector.refresh(now);
			}
		}
	}
};

void Profiler::update(
	const Tracer& tracer,
	const ProfilerConfig& config,
	SteadyTimePoint now,
	bool force_refresh
) {
	++_tick_id;

	std::vector<TraceEvent> stack;
	for (const auto& event : tracer.cold_events()) {
		switch (event.kind()) {
			using enum TraceEventKind;
			case StackPush: {
				stack.push_back(event);
				Impl::handle_stack_push(*this, tracer, event);
				break;
			}
			case StackPop: {
				FR_ASSERT(!stack.empty() && stack.back().label_id() == event.label_id());
				auto push_event = stack.back();
				stack.pop_back();
				Impl::handle_stack_pop(*this, stack, push_event, event);
				break;
			}
			case Point:
				break;
		}
	}

	if (force_refresh)
		Impl::refresh<true>(*this, config, now);
	else
		Impl::refresh<false>(*this, config, now);
}

} // namespace fr
