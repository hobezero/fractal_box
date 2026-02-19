#ifndef FRACTAL_BOX_RUNTIME_CORE_PRESET_HPP
#define FRACTAL_BOX_RUNTIME_CORE_PRESET_HPP

#include <glm/vec2.hpp>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/timeline.hpp"
#include "fractal_box/runtime/runtime.hpp"
#include "fractal_box/runtime/tracer_profiler.hpp"

namespace fr {

// Non-repeating phases
// --------------------
struct OneShotPhaseBase { using IsOneShot = TrueC; };

struct BuildPhase: OneShotPhaseBase { };
struct SetupPhase: OneShotPhaseBase { };
struct ShutdownPhase: OneShotPhaseBase { };

using OneShotPhases = MpList<
	BuildPhase,
	SetupPhase,
	ShutdownPhase
>;

// Repeating (loop) phases
// -----------------------

struct FrameFirstPhase { };
struct FrameStartPhase { };

struct LoopPreparePhase { };

struct FixedUpdateEarlyPhase { };
struct FixedUpdateMainPhase { };
struct FixedUpdateLatePhase { };

struct LoopUpdateEarlyPhase { };
struct LoopUpdateMainPhase { };
struct LoopUpdateLatePhase { };

struct LoopRenderEarlyPhase { };
struct LoopRenderMainPhase { };
struct LoopRenderLatePhase { };

struct FrameRenderEarlyPhase { };
struct FrameRenderMainPhase { };
struct FrameRenderLatePhase { };

struct FrameEndPhase { };
struct FrameLastPhase { };

using LoopPhases = MpList<
	FrameFirstPhase,
	FrameStartPhase,

	LoopPreparePhase,

	FixedUpdateEarlyPhase,
	FixedUpdateMainPhase,
	FixedUpdateLatePhase,

	LoopUpdateEarlyPhase,
	LoopUpdateMainPhase,
	LoopUpdateLatePhase,

	LoopRenderEarlyPhase,
	LoopRenderMainPhase,
	LoopRenderLatePhase,

	FrameRenderEarlyPhase,
	FrameRenderMainPhase,
	FrameRenderLatePhase,

	FrameEndPhase,
	FrameLastPhase
>;

// Messages
// --------

struct TickAtFrameStart { using TickAt = FrameStartPhase; };
struct TickAtFrameEnd { using TickAt = FrameEndPhase; };
struct TickAtFixedUpdate { using TickAt = FixedUpdateMainPhase; };
struct TickAtLoopUpdate { using TickAt = LoopUpdateMainPhase; };

struct ReqLoopInterrupt: TickAtFrameEnd { };
struct ReqLoopContinue: TickAtFrameEnd { };
struct ReqLoopToggle: TickAtFrameEnd { };
struct ReqLoopStep: TickAtFrameEnd { bool with_const_delta = false; };
struct ReqLoopQuit: TickAtFrameEnd { static constexpr auto ttl = MessageTtl::Persistent; };

using LoopRequests = MpList<
	ReqLoopInterrupt,
	ReqLoopContinue,
	ReqLoopToggle,
	ReqLoopStep,
	ReqLoopQuit
>;

struct WindowResized: TickAtLoopUpdate {
	glm::ivec2 window_size;
	glm::ivec2 framebuffer_size;
};

using WindowMessages = MpList<WindowResized>;

enum class LoopStatus: uint8_t {
	/// @brief Run frames one after another
	Flow,
	/// @brief Run one frame, then stop
	Step,
	/// @brief Paused in debug mode
	Interrupted,
};

inline constexpr
auto to_string_view(LoopStatus status) noexcept -> std::string_view {
	using enum LoopStatus;
	switch (status) {
		case Flow: return "Flow";
		case Step: return "Step";
		case Interrupted: return "Interrupted";
	}
	FR_UNREACHABLE();
}

inline constexpr
auto is_loop_advancing(LoopStatus status) noexcept -> bool {
	using enum LoopStatus;
	switch (status) {
		case Flow: case Step: return true;
		case Interrupted: return false;
	}
	FR_UNREACHABLE();
}

struct LoopConfig {
	static
	auto make() noexcept -> LoopConfig;

public:
	RawDuration step_delta;
	RawDuration min_app_delta;
	RawDuration max_app_delta;
	RawDuration fixed_delta;
	double time_scale;
	RawDuration profiler_refresh_period;
};

struct FixedClock: Stopwatch<FixedClock, AppEpochTag> { };

static_assert(c_clock<FixedClock>);

struct BasicRunner {
	using MessageTickPhases = MpList<
		FrameStartPhase,
		FrameEndPhase,
		FixedUpdateMainPhase,
		LoopUpdateMainPhase
	>;

	static
	auto run(
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
	) -> ErrorOr<>;
};

struct CorePreset {
	static constexpr auto frame_iter_label = HashedStrView{"$FrameIter"};
	static constexpr auto profiler_update_label = HashedStrView{"$ProfilerUpdate"};
	static constexpr auto message_tick_label = HashedStrView{"$MessageManagerTick"};

	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif // include guard
