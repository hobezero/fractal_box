#include "fractal_box/runtime/runtime.hpp"

#include "fractal_box/core/scope.hpp"

namespace fr {

Runtime::Runtime(int argc, char* argv[]) {
	add_part(ProcessArgs{.argv = {argv, static_cast<size_t>(argc)}});
}

auto Runtime::add_phase(AnyPhase phase) -> Runtime& {
	auto result = _phases.try_emplace_at(phase.type_idx(), std::move(phase));
	const auto& new_phase = result.where->second;
	FR_PANIC_CHECK_FMT(result, "Runtime: phase '{}' already exists", new_phase.name());

	FR_LOG_INFO("Runtime: added phase '{}' with type_index = {}", result.where->second.name(),
		new_phase.type_idx().value());

	_tracer.register_label(new_phase.type_info().name_id(), new_phase.name());

	return *this;
}

auto Runtime::add_system(PhaseToken phase_token, AnySystem system) -> Runtime& {
	auto* phase = _phases.try_get_at(phase_token.type_idx());
	FR_PANIC_CHECK_FMT(phase, "Runtime: phase '{}' doesn't exist",
		phase_token.type_info().custom_name());

	auto result = _systems.try_emplace_at(system.type_idx(), std::move(system));
	auto& new_system = result.where->second;
	FR_PANIC_CHECK_FMT(result, "Runtime: system '{}' already exists", new_system.name());

	phase->append_system(new_system.make_token());
	FR_LOG_INFO("Runtime: added system '{}' to phase '{}'", new_system.name(),
		phase->name());

	_tracer.register_label(new_system.type_info().name_id(), new_system.name());

	return *this;
}

auto Runtime::run_system(SystemToken token, const RunSystemConfig& cfg) -> ErrorOr<bool> {
	auto& system = get_system(token);
	if (!system.is_enabled() || !system.should_run(*this))
		return false;

	if (cfg.log_enabled)
		FR_LOG_TRACE("Runtime: running system '{}'...", system.name());

	const auto trace_guard = _tracer.trace_stack_push(system.type_info().name_id(),
		TraceEventUserOpts{.semantics = TraceEventSemantics::System,
			.index = system.type_idx().value()});

	_curr_system = token.type_idx();
	FR_DEFER [&] {
		system.set_last_invoke_info(_real_clock.tick_id(), SteadyClock::now());
		_curr_system = SystemTypeIdx{};
	};

	if (auto result = system.invoke_run(*this); !result)
		return extract_unexpected(std::move(result));

	return true;
}

auto Runtime::run_phase(PhaseToken token) -> ErrorOr<> {
	auto& phase = get_phase(token);

	const auto trace_guard = _tracer.trace_stack_push(phase.type_info().name_id(),
		TraceEventUserOpts{.semantics = TraceEventSemantics::Phase,
			.index = phase.type_idx().value()});

	if (phase.is_one_shot())
		FR_LOG_TRACE("Runtime: running phase '{}'...", phase.name());

	_curr_phase = token.type_idx();
	FR_DEFER [&] {
		// phase.set_last_invoke_info(_real_clock.tick_id(), SteadyClock::now());
		_curr_phase = {};
	};

	phase.invoke_pre_run_hooks(*this);
	const auto run_cfg = RunSystemConfig{.log_enabled = phase.is_one_shot()};
	for (const auto system_token : phase.systems()) {
		if (auto result = run_system(system_token, run_cfg); !result)
			return extract_unexpected(std::move(result));
	}
	phase.invoke_post_run_hooks(*this);

	return {};
}

auto Runtime::run() -> ErrorOr<> {
	FR_PANIC_CHECK_MSG(_runner.has_value(), "Runtime: runner has not been set");
	_tracer.start();
	return _runner->invoke_run(*this);
}

} // namespace fr
