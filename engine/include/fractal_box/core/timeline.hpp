#ifndef FRACTAL_BOX_CORE_TIMELINE_HPP
#define FRACTAL_BOX_CORE_TIMELINE_HPP

#include <chrono>
#include <compare>
#include <concepts>

#include <fmt/format.h>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/chrono_types.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class Tag>
class TickId {
public:
	using ValueType = int64_t;
	using DiffType = std::make_signed_t<ValueType>;

	static constexpr
	auto max() noexcept -> TickId { return TickId{adopt, std::numeric_limits<ValueType>::max()}; }

	TickId() = default;

	explicit FR_FORCE_INLINE constexpr
	TickId(AdoptInit, ValueType value) noexcept: _value{value} { }

	auto operator<=>(this TickId, TickId) = default;

	// Increment operators
	// ^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator++() noexcept -> TickId& {
		++_value;
		return *this;
	}

	FR_FORCE_INLINE constexpr
	auto operator++(int) noexcept -> TickId {
		const auto old = *this;
		++_value;
		return old;
	}

	// Decrement operators
	// ^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator--() noexcept -> TickId& {
		--_value;
		return *this;
	}

	FR_FORCE_INLINE constexpr
	auto operator--(int) noexcept -> TickId {
		const auto old = *this;
		--_value;
		return old;
	}

	// TickId/DiffType assignment with addition
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator+=(DiffType b) noexcept -> TickId& {
		// NOTE: Can't cast `b` to `ValueType` because `ValueType` might be unsigned and `b` might
		// be negative
		_value = static_cast<ValueType>(static_cast<DiffType>(_value) + b);
		return *this;
	}

	// TickId/unsigned_integral assignment with addition
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<std::unsigned_integral B>
	requires (sizeof(B) <= sizeof(ValueType))
	FR_FORCE_INLINE constexpr
	auto operator+=(B b) noexcept -> TickId& {
		_value += static_cast<ValueType>(b);
		return *this;
	}

	// TickId/DiffType assignment with subtraction
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	auto operator-=(DiffType b) noexcept -> TickId& {
		_value = static_cast<ValueType>(static_cast<DiffType>(_value) - b);
		return *this;
	}

	// TickId/unsigned_integral assignment with subtraction
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<std::unsigned_integral B>
	requires (sizeof(B) <= sizeof(ValueType))
	FR_FORCE_INLINE constexpr
	auto operator-=(B b) noexcept -> TickId& {
		_value -= static_cast<ValueType>(b);
		return *this;
	}

	// TickId/TickId subtraction
	// ^^^^^^^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	friend
	auto operator-(TickId a, TickId b) noexcept -> DiffType {
		return static_cast<DiffType>(a._value) - static_cast<DiffType>(b._value);
	}

	// TickId/DiffType addition
	// ^^^^^^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	friend
	auto operator+(TickId a, DiffType b) noexcept -> TickId {
		return TickId{adopt, static_cast<ValueType>(static_cast<DiffType>(a._value) + b)};
	}

	FR_FORCE_INLINE constexpr
	friend
	auto operator+(DiffType a, TickId b) noexcept -> TickId {
		return TickId{adopt, static_cast<ValueType>(static_cast<DiffType>(b._value) + a)};
	}

	// TickId/unsigned_integral addition
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<std::unsigned_integral B>
	requires (sizeof(B) <= sizeof(ValueType))
	FR_FORCE_INLINE constexpr
	friend
	auto operator+(TickId a, B b) noexcept -> TickId {
		return TickId{adopt, a._value + static_cast<ValueType>(b)};
	}

	template<std::unsigned_integral A>
	requires (sizeof(A) <= sizeof(ValueType))
	FR_FORCE_INLINE constexpr
	friend
	auto operator+(A a, TickId b) noexcept -> TickId {
		return TickId{adopt, b._value + static_cast<ValueType>(a)};
	}

	// TickId/DiffType subtraction
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

	FR_FORCE_INLINE constexpr
	friend
	auto operator-(TickId a, DiffType b) noexcept -> TickId {
		return TickId{adopt, static_cast<ValueType>(static_cast<DiffType>(a._value) - b)};
	}

	// TickId/unsigned_integral subtraction
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	template<std::unsigned_integral B>
	requires (sizeof(B) <= sizeof(ValueType))
	FR_FORCE_INLINE constexpr
	friend
	auto operator-(TickId a, B b) noexcept -> TickId {
		return TickId{adopt, static_cast<ValueType>(a._value - static_cast<ValueType>(b))};
	}

	// Getters
	// ^^^^^^^

	FR_FORCE_INLINE constexpr
	auto value(this TickId self) noexcept -> ValueType { return self._value; }

private:
	ValueType _value = 0;
};

class Timeline {
public:
	using TimePoint = SteadyTimePoint;
	using TickIdType = TickId<Timeline>;

	/// @brief Creates stopped timeline
	explicit
	Timeline() noexcept = default;

	auto start() noexcept {
		_start_tpoint = SteadyClock::now();
		_last_tick_tpoint = _start_tpoint;
		_is_running = true;
	}

	void tick() noexcept {
		FR_ASSERT(_is_running);
		const auto now = SteadyClock::now();
		_prev_tick_duration = now - _last_tick_tpoint;
		_last_tick_tpoint = now;
		++_tick_id;
		if (_tick_id == TickIdType::max())
			FR_LOG_WARN_MSG("Timeline: tick_id overflow");
	}

	auto elapsed() const noexcept -> RawDuration {
		return _last_tick_tpoint - _start_tpoint;
	}

	auto last_tick_tpoint() const noexcept -> SteadyTimePoint { return _last_tick_tpoint; }
	auto last_tick_duration() const noexcept -> RawDuration { return _prev_tick_duration; }
	auto tick_id() const noexcept -> TickIdType { return _tick_id; }

private:
	bool _is_running = true;
	SteadyTimePoint _start_tpoint;
	SteadyTimePoint _last_tick_tpoint;
	RawDuration _prev_tick_duration = RawDuration::zero();
	TickIdType _tick_id;
};

/// @brief Stopwatch which tracks current time and delta time. Delta is provided by the user
/// @note Defined as a separate class so that multiple stopwatches with different `TickIdTag`'s but
/// the same `EpochTag`'s would have the same `TimePoint` type. Standard explicitly allows
/// `C::time_point::clock_type` not to match `C` [time.clock.req]
template<class EpochTag>
struct StopwatchBase {
	using Rep = SteadyClock::rep;
	using Period = SteadyClock::period;
	using Duration = std::chrono::duration<Rep, Period>;
	using TimePoint = std::chrono::time_point<StopwatchBase, Duration>;

	// C++'s official `Cpp17Clock` concept/named requirements
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	using rep = Rep;
	using period = Period;
	using duration = Duration;
	using time_point = TimePoint;
	static constexpr auto is_steady = true;

	auto now() const noexcept -> TimePoint { return TimePoint{_elapsed}; }

	void tick(Duration delta) noexcept {
		_elapsed += delta;
		_delta = delta;
	}

	auto elapsed() const noexcept -> Duration { return _elapsed; }

	FR_FORCE_INLINE
	auto delta() const noexcept -> Duration { return _delta; }

	FR_FORCE_INLINE
	auto f_delta() const noexcept -> FDuration { return chrono_cast<FDuration>(_delta); }

protected:
	Duration _elapsed = Duration::zero();
	Duration _delta = Duration::zero();
};

/// @brief Stopwatch which tracks TickIds, in addition to current time and delta time
template<class TickIdTag, class EpochTag>
class Stopwatch: public StopwatchBase<EpochTag> {
	using Base = StopwatchBase<EpochTag>;

public:
	using TickIdType = TickId<TickIdTag>;

	void tick(Base::Duration delta) noexcept {
		Base::tick(delta);
		++_tick_id;
		if (_tick_id == TickIdType::max())
			FR_LOG_WARN_MSG("Stopwatch: tick_id overflow");
	}

	auto tick_id() const noexcept -> TickIdType { return _tick_id; }

private:
	TickIdType _tick_id;
};

} // namespace fr

template<class Tag>
struct fmt::formatter<fr::TickId<Tag>>: formatter<typename fr::TickId<Tag>::ValueType> {
	auto format(fr::TickId<Tag> id, format_context& ctx) const {
		return formatter<typename fr::TickId<Tag>::ValueType>::format(id.value(), ctx);
	}
};

#endif // include guard
