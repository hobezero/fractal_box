#include "fractal_box/runtime/runtime.hpp"

#include "fractal_box/core/scope.hpp"

namespace fr {

auto handle_diagnostic(Diagnostic diag, std::span<const Diagnostic> context) -> ControlFlow {
	for (auto i = 0zu; i < context.size(); ++i) {
		switch (diag.severity()) {
			case DiagnosticSeverity::Context:
				FR_PANIC();
			case DiagnosticSeverity::Warning:
				FR_LOG_WARN("{:>{}}{}", "", 2 * i, context[i].format());
				break;
			case DiagnosticSeverity::Error:
				FR_LOG_ERROR("{:>{}}{}", "", 2 * i, context[i].format());
				break;
		}
	}
	switch (diag.severity()) {
		case DiagnosticSeverity::Context:
			FR_PANIC();
		case DiagnosticSeverity::Warning:
			FR_LOG_WARN("{:>{}}{}", "", 2 * context.size(), diag.format());
			break;
		case DiagnosticSeverity::Error:
			FR_LOG_ERROR("{:>{}}{}", "", 2 * context.size(), diag.format());
			break;
	}
	return ControlFlow::Continue;
}

auto AnySystem::invoke_run(Runtime &runtime) -> Status<> {
	FR_ASSERT(_run_fn);

	for (auto i = 0uz; i < _pre_run_hooks.size(); ++i) {
		const auto frm = runtime.diagnostic_sink().make_frame(StringContext{[&] {
			return fmt::format("While running pre-run-hook #{} of system '{}':",
				std::to_underlying(_pre_run_hooks.keys()[i]), _name.str());
		}});
		FR_IGNORE(_pre_run_hooks.values()[i](runtime, *this));
	}

	const auto result = _run_fn(runtime, *this);

	for (auto i = 0uz; i < _post_run_hooks.size(); ++i) {
		const auto frm = runtime.diagnostic_sink().make_frame(StringContext{[&] {
			return fmt::format("While running pre-run-hook #{} of system '{}':",
				std::to_underlying(_post_run_hooks.keys()[i]), _name.str());
		}});
		FR_IGNORE(_post_run_hooks.values()[i](runtime, *this));
	}
	return result;
}

void AnyPhase::invoke_pre_run_hooks(Runtime& runtime) {
	for (auto i = 0uz; i < _pre_run_hooks.size(); ++i) {
		const auto frm = runtime.diagnostic_sink().make_frame(StringContext{[&] {
			return fmt::format("While running pre-run-hook #{} of phase '{}':",
				std::to_underlying(_pre_run_hooks.keys()[i]), _name.str());
		}});
		FR_IGNORE(_pre_run_hooks.values()[i](runtime, *this));
	}
}

void AnyPhase::invoke_post_run_hooks(Runtime& runtime) {
	for (auto i = 0uz; i < _post_run_hooks.size(); ++i) {
		const auto frm = runtime.diagnostic_sink().make_frame(StringContext{[&] {
			return fmt::format("While running post-run-hook #{} of phase '{}':",
				std::to_underlying(_post_run_hooks.keys()[i]), _name.str());
		}});
		FR_IGNORE(_post_run_hooks.values()[i](runtime, *this));
	}
}

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

auto Runtime::run_system(SystemToken token, const RunSystemConfig& cfg) -> Status<bool> {
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
		return from_error;

	return true;
}

auto Runtime::run_phase(PhaseToken token) -> Status<> {
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
			return from_error;
	}
	phase.invoke_post_run_hooks(*this);

	return {};
}

auto Runtime::run() -> Status<> {
	FR_PANIC_CHECK_MSG(_runner.has_value(), "Runtime: runner has not been set");
	_tracer.start();
	return _runner->invoke_run(*this);
}

} // namespace fr
