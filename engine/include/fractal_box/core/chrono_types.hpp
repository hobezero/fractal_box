#ifndef FRACTAL_BOX_CORE_CHRONO_TYPES_HPP
#define FRACTAL_BOX_CORE_CHRONO_TYPES_HPP

#include <chrono>
#include <limits>
#include <type_traits>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

template<class T>
concept c_ratio = c_specialization_nttp<T, std::ratio>;

template<class T>
concept c_duration = c_specialization<T, std::chrono::duration>;

/// @brief Exactly the same as `Cpp17Clock`, except stateful clocks are allowed (`T::now()` can be
/// non-static).
/// @see P2212R2, https://eel.is/c++draft/time.point
template<class T>
concept c_clock = requires(T obj) {
	typename T::rep;
	typename T::period;
	typename T::duration;
	typename T::time_point::clock;
	typename T::time_point::duration;
	{ &T::is_steady } -> std::same_as<const bool*>;
	{ obj.now() } -> std::same_as<typename T::time_point>;
	requires std::same_as<typename T::duration,
		std::chrono::duration<typename T::rep, typename T::period>>;
	requires std::same_as<typename T::time_point::duration, typename T::duration>;
};

using SteadyClock = typename MpLazyIf<std::chrono::high_resolution_clock::is_steady>::template Type<
	std::chrono::high_resolution_clock,
	std::chrono::steady_clock
>;
static_assert(SteadyClock::is_steady);

template<c_arithmetic Rep, c_ratio Period = std::ratio<1>>
using Duration = std::chrono::duration<Rep, Period>;

/// @brief Time delta with the highest posible resolution. Duration is (usually) in nanosecors
/// stored as an int64_t
/// @warning Be careful with integer arithmetic. Division may do something you don't want
using RawDuration = SteadyClock::duration;
static_assert(std::ratio_less_equal_v<RawDuration::period, std::micro>);
using DRawDuration = Duration<double, RawDuration::period>;

/// @brief Default type for time deltas. Duration in seconds stored as a float
using FDuration = std::chrono::duration<float, std::ratio<1>>;
using DDuration = std::chrono::duration<double, std::ratio<1>>;

using FDurationMs = std::chrono::duration<float, std::milli>;
using DDurationMs = std::chrono::duration<double, std::milli>;

template<c_clock ClockType, c_duration DurationType = typename ClockType::duration>
using TimePoint = std::chrono::time_point<ClockType, DurationType>;

/// @brief Default type for points in time
using SteadyTimePoint = SteadyClock::time_point;

template<c_duration Duration>
inline constexpr auto duration_lowest = Duration{
	std::numeric_limits<typename Duration::rep>::lowest()};

template<c_duration Duration>
inline constexpr auto duration_max = Duration{
	std::numeric_limits<typename Duration::rep>::max()};

template<c_duration TargetDuration, class SourceRep, class SourcePeriod>
FR_FORCE_INLINE constexpr
auto chrono_cast(const Duration<SourceRep, SourcePeriod>& from) -> TargetDuration {
	return std::chrono::duration_cast<TargetDuration>(from);
}

/// @brief Convenience wrapper for `std::chrono::duration_cast(..)`. Change period
template<c_ratio TargetPeriod, class SourceRep, class SourcePeriod>
FR_FORCE_INLINE constexpr
auto chrono_cast(Duration<SourceRep, SourcePeriod> from) -> Duration<SourceRep, TargetPeriod> {
	return std::chrono::duration_cast<Duration<SourceRep, TargetPeriod>>(from);
}

/// @brief Convenience wrapper for std::chrono::duration_cast(..)
template<
	c_arithmetic TargetRep, c_ratio TargetPeriod = std::ratio<1>,
	class SourceRep, class SourcePeriod
>
FR_FORCE_INLINE constexpr
auto chrono_cast(Duration<SourceRep, SourcePeriod> from) -> Duration<TargetRep, TargetPeriod> {
	return std::chrono::duration_cast<Duration<TargetRep, TargetPeriod>>(from);
}

template<c_arithmetic TargetRep, c_ratio TargetPeriod, class SourceClock, class SourceDuration>
FR_FORCE_INLINE constexpr
auto time_cast(
	TimePoint<SourceClock, SourceDuration> from
) -> TimePoint<SourceClock, Duration<TargetRep, TargetPeriod>> {
	return std::chrono::time_point_cast<Duration<TargetRep, TargetPeriod>>(from);
}

/// @todo
///   TODO: Require that std::same_as<SourceClock::EpochTag, TargetClock::EpochTag>
///   TODO: Look into `std::chrono::clock_time_conversion`, `std::chrono::clock_cast`
template<c_clock TargetClock, class SourceClock, class SourceDuration>
FR_FORCE_INLINE constexpr
auto clock_time_cast(
	TimePoint<SourceClock, SourceDuration> from
) -> TimePoint<TargetClock, SourceDuration> {
	return TimePoint<TargetClock, SourceDuration>{from.time_since_epoch()};
}

template<c_arithmetic T>
using ExtendedDurationRep = typename MpLazyIf<std::is_integral_v<T>>::template Type<
	RawDuration::rep, T>;

template<c_arithmetic T, c_ratio Period>
using ExtendedDuration = Duration<ExtendedDurationRep<T>, Period>;

/// Construct time duration in hours
template<class T>
FR_FORCE_INLINE constexpr
auto hours(T value) noexcept {
	return ExtendedDuration<T, std::ratio<3600, 1>>{ExtendedDurationRep<T>{value}};
}

/// Construct time duration in minutes
template<class T>
FR_FORCE_INLINE constexpr
auto minutes(T value) noexcept {
	return ExtendedDuration<T, std::ratio<60, 1>>{ExtendedDurationRep<T>{value}};
}

/// Construct time duration in seconds
template<class T>
FR_FORCE_INLINE constexpr
auto secs(T value) noexcept {
	return ExtendedDuration<T, std::ratio<1>>{ExtendedDurationRep<T>{value}};
}

/// Construct time duration in milliseconds
template<class T>
FR_FORCE_INLINE constexpr
auto msecs(T value) noexcept {
	return ExtendedDuration<T, std::milli>{ExtendedDurationRep<T>{value}};
}

/// Construct time duration in microseconds
template<class T>
FR_FORCE_INLINE constexpr
auto usecs(T value) noexcept {
	return ExtendedDuration<T, std::micro>{ExtendedDurationRep<T>{value}};
}

/// Construct time duration in nanoseconds
template<class T>
FR_FORCE_INLINE constexpr
auto nsecs(T value) noexcept {
	return ExtendedDuration<T, std::nano>{ExtendedDurationRep<T>{value}};
}

} // namespace fr
#endif // include guard
