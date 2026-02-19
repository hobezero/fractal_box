#include "fractal_box/graphics/debug_draw_preset.hpp"

#include <algorithm>

#include "fractal_box/graphics/debug_draw_config.hpp"
#include "fractal_box/runtime/core_preset.hpp"

namespace fr {

struct DebugDrawAdHoc::ClearExpiredDebugShapesSystem {
	static
	auto condition(LoopStatus status) noexcept -> bool {
		return is_loop_advancing(status);
	}

	static
	void run(DebugDrawAdHocData& data, const AppClock& app_clock) {
		// Partition splits `shape_infos` into two groups: lifetimes that should be carried over
		// into the next frame and lifetimes that have already expired. The second group gets
		// erased from the original containers
		for_each_shape_vec(data._frame_shape_infos, [&]<class Shape>(auto& shape_infos) {
			const auto expired = std::ranges::partition(shape_infos, [&] (auto& info) {
				return info.lifetime.expiration_tick > app_clock.tick_id();
			});
			auto& shapes = dispatch_shape_type<Shape>(data._shapes);
			for (const auto& info : expired)
				shapes.erase(info.id);
			shape_infos.erase(expired.begin(), expired.end());
		});

		for_each_shape_vec(data._timed_shape_infos, [&]<class Shape>(auto& shape_infos) {
			const auto expired = std::ranges::partition(shape_infos, [&] (auto& info) {
				return info.lifetime.expiration_tpoint > app_clock.now();
			});
			auto& shapes = dispatch_shape_type<Shape>(data._shapes);
			for (const auto& info : expired)
				shapes.erase(info.id);
			shape_infos.erase(expired.begin(), expired.end());
		});
	}
};

void DebugDrawPreset::build(Runtime& runtime) {
	runtime
		.try_add_part(DebugDrawConfig{})
		.add_part(DebugDrawAdHocData{})
		.add_system<FrameEndPhase, DebugDrawAdHoc::ClearExpiredDebugShapesSystem>()
	;
}

} // namespace fr
