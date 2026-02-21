#ifndef FRACTAL_BOX_GRAPHICS_DEBUG_DRAW_PRESET_HPP
#define FRACTAL_BOX_GRAPHICS_DEBUG_DRAW_PRESET_HPP

#include <vector>

#include "fractal_box/core/color.hpp"
#include "fractal_box/core/containers/sparse_set.hpp"
#include "fractal_box/core/containers/unordered_map.hpp"
#include "fractal_box/math/shapes.hpp"
#include "fractal_box/runtime/runtime.hpp"
#include "fractal_box/scene/transform.hpp"

namespace fr {

enum class ShapeFill {
	Solid,
	Wire,
};

inline constexpr auto debug_shape_color_default = Color4{color_magenta, 0.95f};
inline constexpr auto debug_shape_z_index_default = 0.9f;

struct DebugLine {
	static constexpr
	auto from_dir(
		glm::vec2 start,
		glm::vec2 direction,
		Color4 color = debug_shape_color_default,
		float z_index = debug_shape_z_index_default
	) noexcept -> DebugLine {
		return {
			.start = start,
			.finish = start + direction,
			.color = color,
			.z_index = z_index,
		};
	}

	auto make_transform() const noexcept -> Transform {
		const auto dir = finish - start;
		return {this->start + 0.5f * dir, vec_rotation(dir), {glm::length(dir), 1.f}};
	}

public:
	glm::vec2 start;
	glm::vec2 finish;
	Color4 color = debug_shape_color_default;
	float z_index = 0.9f;
};

struct DebugRect {
	constexpr
	auto make_transform() const noexcept -> Transform {
		return{this->bounds.center(), {}, this->bounds.size()};
	}

public:
	FAaRect bounds;
	ShapeFill shape_fill = ShapeFill::Wire;
	Color4 color = debug_shape_color_default;
	float z_index = 0.9f;
};

struct DebugCircle {
	constexpr
	auto make_transform() const noexcept -> Transform {
		const auto s = 2.f * this->circle.radius();
		return {this->circle.center(), {}, {s, s}};
	}

public:
	FCircle circle;
	ShapeFill shape_fill = ShapeFill::Wire;
	Color4 color = debug_shape_color_default;
	float z_index = 0.9f;
};

struct DebugPoint {
	constexpr
	auto make_transform(float scale_factor) const noexcept -> Transform {
		const auto s = 2.f * scale_factor * this->radius_pixels;
		return {this->coords, {}, {s, s}};
	}

public:
	glm::vec2 coords;
	float radius_pixels = 2.f;
	Color4 color = debug_shape_color_default;
	float z_index = 0.9f;
};

enum class DebugShapeId: uint32_t { };

/// @brief A very simple hand-rolled ECS for debug shapes specifically
/// @warning Do not directly write to this data structure, use `DebugDrawAdHoc` part instead
/// @note Versionless SparseMap workds as long as there is only one key stored per shape
/// in any of the lifetimes
class DebugDrawAdHocData {
public:
	struct Shapes {
		SparseMapVersionless<DebugShapeId, DebugLine> lines;
		SparseMapVersionless<DebugShapeId, DebugRect> rects;
		SparseMapVersionless<DebugShapeId, DebugCircle> circles;
		SparseMapVersionless<DebugShapeId, DebugPoint> points;
	};

	struct LifetimeTillFrame { AppClock::TickIdType expiration_tick; };
	struct LifetimeTillTime { AppClock::TimePoint expiration_tpoint; };
	struct LifetimeSystemRun { SystemTypeIdx system_idx; };
	struct LifetimePhaseRun { PhaseTypeIdx phase_idx; };
	struct LifetimeById { uint32_t custom_id; };


	template<class L>
	struct ShapeInfo {
		DebugShapeId id;
		L lifetime;
	};

	template<class L>
	struct ShapeInfoPack {
		std::vector<ShapeInfo<L>> lines;
		std::vector<ShapeInfo<L>> rects;
		std::vector<ShapeInfo<L>> circles;
		std::vector<ShapeInfo<L>> points;
	};

	struct ShapeIdPack {
		std::vector<DebugShapeId> lines;
		std::vector<DebugShapeId> rects;
		std::vector<DebugShapeId> circles;
		std::vector<DebugShapeId> points;
	};

	friend class DebugDrawAdHoc;

public:
	auto shapes() const noexcept -> const Shapes& { return _shapes; }

private:
	// Actual data for shapes
	Shapes _shapes;

	// "Accelerated" data structures to quickly remove shapes that ran out of lifetime
	// -------------------------------------------------------------------------------
	ShapeInfoPack<LifetimeTillFrame> _frame_shape_infos;
	ShapeInfoPack<LifetimeTillTime> _timed_shape_infos;
	UnorderedMap<SystemTypeIdx, ShapeIdPack> _system_shape_ids;
	UnorderedMap<PhaseTypeIdx, ShapeIdPack> _phase_shape_ids;
};

class DebugDrawAdHoc: public EphemeralPartBase {
public:
	explicit
	DebugDrawAdHoc(Runtime& runtime, AnySystem& system) noexcept:
		DebugDrawAdHoc{runtime, unwrap(runtime.current_phase()), system}
	{ }

	/// @note Since this constructor touches phase & system data, it will introduce a data race
	/// when we implement multithreaded system execution. The issue can be fixed by delaying
	/// hook insertion through upcoming "command queue" API
	explicit
	DebugDrawAdHoc(Runtime& runtime, AnyPhase& phase, AnySystem& system) noexcept:
		_app_clock{&runtime.get_part<AppClock>()},
		_phase_idx{phase.type_idx()},
		_system_idx{system.type_idx()},
		_data{&runtime.get_part<DebugDrawAdHocData>()}
	{
		if (!_data->_system_shape_ids.contains(_system_idx)) {
			system.pre_run_hooks().insert(
				Runtime::make_system_routine<clear_system_debug_shapes>());
		}
		if (!_data->_phase_shape_ids.contains(_phase_idx)) {
			phase.pre_run_hooks().insert(Runtime::make_phase_routine<clear_phase_debug_shapes>());
		}
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_for_frame(Shape&& shape) {
		add_for_n_frames(std::forward<Shape>(shape), 1);
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_for_n_frames(Shape&& shape, int32_t frame_count) {
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimeTillFrame{
			_app_clock->tick_id() + frame_count});
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_util_frame(Shape&& shape, AppClock::TickIdType frame_id) {
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimeTillFrame{frame_id});
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_for_duration(Shape&& shape, c_duration auto duration) {
		const auto until = _app_clock->now() + chrono_cast<AppClock::Duration>(duration);
		FR_LOG_DEBUG("!!! now: {:.3f}, until: {:.3f}",
			chrono_cast<FDuration>(_app_clock->elapsed()).count(),
			chrono_cast<FDuration>(until.time_since_epoch()).count()
		);
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimeTillTime{
			_app_clock->now() + chrono_cast<AppClock::Duration>(duration)});
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_until(Shape&& shape, AppClock::TimePoint tpoint) {
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimeTillTime{tpoint});
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_to_system(Shape&& shape) {
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimeSystemRun{_system_idx});
	}

	template<class Shape>
	FR_FORCE_INLINE
	void add_to_phase(Shape&& shape) {
		add_impl(std::forward<Shape>(shape), DebugDrawAdHocData::LifetimePhaseRun{_phase_idx});
	}

private:
	struct ClearExpiredDebugShapesSystem;
	friend struct DebugDrawPreset;

	template<class S>
	static FR_FORCE_INLINE
	auto dispatch_shape_type(auto&& pack) -> auto&& {
		if constexpr (std::is_same_v<S, DebugLine>) return pack.lines;
		else if constexpr (std::is_same_v<S, DebugRect>) return pack.rects;
		else if constexpr (std::is_same_v<S, DebugCircle>) return pack.circles;
		else if constexpr (std::is_same_v<S, DebugPoint>) return pack.points;
		else static_assert(false, "Unsupported DebugShape");
	}

	static
	void for_each_shape_vec(auto&& pack, auto&& callback) {
		callback.template operator()<DebugLine>(pack.lines);
		callback.template operator()<DebugRect>(pack.rects);
		callback.template operator()<DebugCircle>(pack.circles);
		callback.template operator()<DebugPoint>(pack.points);
	}

	static
	void clear_system_debug_shapes(const AnySystem& system, DebugDrawAdHocData& data) {
		auto& sys_shape_ids = data._system_shape_ids[system.type_idx()];
		for_each_shape_vec(sys_shape_ids, [&]<class Shape>(auto& shape_ids) {
			auto& shapes = dispatch_shape_type<Shape>(data._shapes);
			for (auto id : shape_ids)
				shapes.erase(id);
			shape_ids.clear();
		});
	}

	static
	void clear_phase_debug_shapes(const AnyPhase& phase, DebugDrawAdHocData& data) {
		auto& phase_shape_ids = data._phase_shape_ids[phase.type_idx()];
		for_each_shape_vec(phase_shape_ids, [&]<class Shape>(auto& shape_ids) {
			auto& shapes = dispatch_shape_type<Shape>(data._shapes);
			for (auto id : shape_ids)
				shapes.erase(id);
			shape_ids.clear();
		});
	}

	template<class Shape, class Lifetime>
	void add_impl(Shape&& shape, Lifetime&& lifetime) {
		using S = std::remove_cvref_t<Shape>;
		using L = std::remove_cvref_t<Lifetime>;

		auto& shapes = dispatch_shape_type<S>(_data->_shapes);
		auto shape_id = shapes.insert(std::forward<Shape>(shape));

		if constexpr (std::is_same_v<L, DebugDrawAdHocData::LifetimeTillFrame>) {
			auto& infos = dispatch_shape_type<S>(_data->_frame_shape_infos);
			infos.emplace_back(shape_id, lifetime);
		}
		else if constexpr (std::is_same_v<L, DebugDrawAdHocData::LifetimeTillTime>) {
			auto& infos = dispatch_shape_type<S>(_data->_timed_shape_infos);
			infos.emplace_back(shape_id, lifetime);
		}
		else if constexpr (std::is_same_v<L, DebugDrawAdHocData::LifetimeSystemRun>) {
			auto& ids = dispatch_shape_type<S>(_data->_system_shape_ids[lifetime.system_idx]);
			ids.push_back(shape_id);
		}
		else if constexpr (std::is_same_v<L, DebugDrawAdHocData::LifetimePhaseRun>) {
			auto& ids = dispatch_shape_type<S>(_data->_phase_shape_ids[lifetime.phase_idx]);
			ids.push_back(shape_id);
		}
		else
			static_assert(false, "Unsupported DebugShape lifetime");
	}

private:
	AppClock* _app_clock;
	PhaseTypeIdx _phase_idx;
	SystemTypeIdx _system_idx;
	DebugDrawAdHocData* _data;
};

static_assert(c_ephemeral_part<DebugDrawAdHoc>);

struct DebugDrawPreset {
	static
	void build(Runtime& runtime);
};

} // namespace fr
#endif
