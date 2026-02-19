#include "fractal_box/runtime/core_preset.hpp"

#include "fractal_box/core/math.hpp"
#include "fractal_box/core/platform.hpp"

FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS

namespace fr {

auto LoopConfig::make() noexcept -> LoopConfig {
	using namespace std::chrono_literals;
	return {
		.step_delta = 1s / 60,
		.min_app_delta = 1ms / 10,
		.max_app_delta = 100ms,
		.fixed_delta = 1s / 80,
		.time_scale = 1.,
		.profiler_refresh_period = 750ms,
	};
}

static
void continue_flow(LoopStatus& status, AppTickId tick_id) noexcept {
	if (status == LoopStatus::Interrupted || status == LoopStatus::Step) {
		status = LoopStatus::Flow;
		FR_LOG_INFO("Loop Continued on frame #{}", tick_id);
	}
}

static
void interrupt(LoopStatus& status, AppTickId tick_id) noexcept {
	if (status == LoopStatus::Flow || status == LoopStatus::Step) {
		status = LoopStatus::Interrupted;
		FR_LOG_INFO("Loop Interrupted on frame #{}", tick_id);
	}
}

static
void toggle(LoopStatus& status, AppTickId tick_id) noexcept {
	if (is_loop_advancing(status))
		interrupt(status, tick_id);
	else
		continue_flow(status, tick_id);
}

static
void step(LoopStatus& status, AppTickId tick_id, bool is_time_step_fixed) noexcept {
	if (status == LoopStatus::Flow || status == LoopStatus::Interrupted) {
		status = LoopStatus::Step;
		FR_LOG_INFO("Loop Stepped on frame #{}{}", tick_id,
			is_time_step_fixed ? " with constant time step" : "");
	}
}

static
auto run_one_iter(
	Runtime& runtime,
	Tracer& tracer,
	const ProfilerConfig& profiler_config,
	Profiler& profiler,
	MessageManager& msg_manager,
	const LoopConfig& loop_config,
	LoopStatus& status,
	Timeline& real_clock,
	AppClock& app_clock,
	FixedClock& fixed_clock,
	bool& should_quit
) -> ErrorOr<> {
	tracer.trace_tick();
	const auto trace_frame_guard = tracer.trace_stack_push(CorePreset::frame_iter_label,
		TraceEventUserOpts{.semantics = TraceEventSemantics::Frame, .has_extras = true});

	const auto prev_real_tpoint = real_clock.last_tick_tpoint();
	FR_UNUSED(prev_real_tpoint);
	real_clock.tick();

	if (is_loop_advancing(status)) {
		const auto trace_profile_guard = tracer.trace_stack_push(CorePreset::profiler_update_label,
			TraceEventUserOpts{.semantics = TraceEventSemantics::Section});
		profiler.update(tracer, profiler_config, SteadyClock::now());
	}
	tracer.consume_cold_events();

	if (status == LoopStatus::Step)
		status = LoopStatus::Interrupted;
	auto is_const_step = false;
	msg_manager.make_reader<MessageListReader<LoopRequests>>().for_each_consume(Overload{
		[&](ReqLoopInterrupt) { interrupt(status, app_clock.tick_id()); },
		[&](ReqLoopContinue) { continue_flow(status, app_clock.tick_id()); },
		[&](ReqLoopToggle) { toggle(status, app_clock.tick_id()); },
		[&](ReqLoopStep m) {
			is_const_step = m.with_const_delta;
			step(status, app_clock.tick_id(), m.with_const_delta);
		},
		[&](ReqLoopQuit) { should_quit = true; }
	});

	auto fixed_update_count = 0;
	auto update_count = 0;

	if (is_loop_advancing(status)) {
		const auto unscaled_delta = is_const_step
			? loop_config.step_delta
			// Mininmum delta to prevent division by zero. Maximum delta to prevent huge time skips
			// when debugging / after stutters or fps drops
			: clamp_between(real_clock.last_tick_duration(), loop_config.min_app_delta,
				loop_config.max_app_delta);
		// Go through milliseconds and back to minimize floating-point errors (presumably,
		// didn't test)
		const auto scaled_delta = chrono_cast<AppClock::Duration>(
			chrono_cast<DDurationMs>(unscaled_delta) * loop_config.time_scale);
		app_clock.tick(scaled_delta);
		++update_count;
	}

	auto res = ErrorOr<>{}; // Forcing NRVO
	if (res = runtime.run_phase<FrameFirstPhase>(); !res) return res;
	if (res = runtime.run_phase<FrameStartPhase>(); !res) return res;

	if (is_loop_advancing(status)) {
		if (res = runtime.run_phase<LoopPreparePhase>(); !res) return res;

		while (fixed_clock.now() + loop_config.fixed_delta <= app_clock.now()) {
			fixed_clock.tick(loop_config.fixed_delta);
			++fixed_update_count;
			if (res = runtime.run_phase<FixedUpdateEarlyPhase>(); !res) return res;
			if (res = runtime.run_phase<FixedUpdateMainPhase>(); !res) return res;
			if (res = runtime.run_phase<FixedUpdateLatePhase>(); !res) return res;
		}

		if (res = runtime.run_phase<LoopUpdateEarlyPhase>(); !res) return res;
		if (res = runtime.run_phase<LoopUpdateMainPhase>(); !res) return res;
		if (res = runtime.run_phase<LoopUpdateLatePhase>(); !res) return res;

		if (res = runtime.run_phase<LoopRenderEarlyPhase>(); !res) return res;
		if (res = runtime.run_phase<LoopRenderMainPhase>(); !res) return res;
		if (res = runtime.run_phase<LoopRenderLatePhase>(); !res) return res;
	}

	if (res = runtime.run_phase<FrameRenderEarlyPhase>(); !res) return res;
	if (res = runtime.run_phase<FrameRenderMainPhase>(); !res) return res;
	if (res = runtime.run_phase<FrameRenderLatePhase>(); !res) return res;

	if (res = runtime.run_phase<FrameEndPhase>(); !res) return res;
	if (res = runtime.run_phase<FrameLastPhase>(); !res) return res;

	{
		const auto trace_msg_guard = tracer.trace_stack_push(CorePreset::message_tick_label,
			TraceEventUserOpts{.semantics = TraceEventSemantics::Section});
		msg_manager.tick<FixedUpdateMainPhase>(fixed_update_count);
		msg_manager.tick<LoopUpdateMainPhase>(update_count);
		msg_manager.tick<FrameEndPhase>();
	}

	return res;
}

auto BasicRunner::run(
	Runtime& runtime,
	Tracer& tracer,
	const ProfilerConfig& profiler_config,
	Profiler& profiler,
	MessageManager& msg_manager,
	const LoopConfig& loop_config,
	LoopStatus& status,
	Timeline& real_clock,
	AppClock& app_clock,
	FixedClock& fixed_clock
) -> ErrorOr<> {
	FR_LOG_INFO_MSG("BasicRunner: starting...");
	auto res = ErrorOr<>{};
	if (res = runtime.run_phase<BuildPhase>(); !res) return res;
	if (res = runtime.run_phase<SetupPhase>(); !res) return res;

	const bool is_loop_empty = [&]<class... Ps>(MpList<Ps...>) {
		return (runtime.get_phase<Ps>().empty() && ...);
	}(LoopPhases{});

	if (is_loop_empty) {
		FR_LOG_WARN_MSG("BasicRunner: there are no loop systems");
	}
	else {
		FR_LOG_INFO_MSG("BasicRunner: running loop systems...");
		real_clock.start();
		auto should_quit = false;
		while (!should_quit) {
			res = run_one_iter(
				runtime, tracer, profiler_config, profiler, msg_manager,
				loop_config, status, real_clock, app_clock, fixed_clock,
				should_quit
			);
			if (!res)
				return res;
		}
	}

	FR_LOG_INFO_MSG("BasicRunner: shutting down...");
	if (res = runtime.run_phase<ShutdownPhase>(); !res) return res;

	return res;
}

void CorePreset::build(Runtime& runtime) {
	runtime
		.add_phases<OneShotPhases>()
		.add_phases<LoopPhases>()
		.try_add_part(ProfilerConfig::make())
		.add_part(Profiler{})
		.add_part(LoopStatus::Flow)
		.try_add_part(LoopConfig::make())
		.add_part(AppClock{})
		.add_part(FixedClock{})
		.set_runner<BasicRunner>()
	;
	runtime.message_manager().register_tick_phases<BasicRunner::MessageTickPhases>();
	runtime.tracer().register_label(HashedStr{CorePreset::frame_iter_label});
	runtime.tracer().register_label(HashedStr{CorePreset::profiler_update_label});
	runtime.tracer().register_label(HashedStr{CorePreset::message_tick_label});
}

} // namespace fr
