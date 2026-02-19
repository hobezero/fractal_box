#ifndef FRACTAL_BOX_CORE_RANDOM_HPP
#define FRACTAL_BOX_CORE_RANDOM_HPP

#include <algorithm>
#include <concepts>
#include <random>

#include <glm/vec2.hpp>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"

namespace fr {

/// @brief Seed sequence powered by std::random_device.
/// Generates unsigned 32-bit integer values by forwarding data from std::random_device.
/// Qualifies as SeedSequence
/// @todo TODO: Remove
class RdSeedSequence {
public:
	using result_type = std::uint_least32_t;

	explicit RdSeedSequence(std::random_device& rd) noexcept : _rd(&rd) { }

	/// @brief Required by SeedSequence. Doesn't do anything
	/// @warning Please don't call it or generate won't be useful
	template<class IntType>
	[[deprecated("Use (std::random_device&) constructor instead")]]
	explicit RdSeedSequence(std::initializer_list<IntType>) noexcept { }

	/// @brief Required by SeedSequence. Doesn't do anything
	/// @warning Please don't call it or generate won't be useful
	template<class InputIt>
	[[deprecated("Use (std::random_device&) constructor instead")]]
	RdSeedSequence(InputIt, InputIt) noexcept { }

	template<class RandomAccessIt>
	void generate(RandomAccessIt begin, RandomAccessIt end) {
		if (_rd) {
			std::generate(begin, end, [rd = this->_rd] {
				return (*rd)();
			});
		}
		else {
			// 0x8B8B8B8B is used by std::seed_seq
			// TODO: Is there a better strategy? Precondition with assert() or std::exit(), maybe?
			std::fill(begin, end, std::random_device::result_type{UINT32_C(0x8B8B8B8B)});
		}
	}

	constexpr auto size() const noexcept -> size_t { return 0; }

	template<typename OutputIt>
	constexpr void param(OutputIt) const noexcept { }

private:
	std::random_device* _rd{};
};

using DefaultRandomEngine = std::mt19937_64;

/// @note According to the standard, will produce values in a closed range for integral types
/// and a half-open range for floating point types. In practice, the precision of floating point
/// distributions is not that high and the rounding errors may cause this function to return `max`
/// anyway. Not sure if the issue is worth fixing. Some links:
///   - https://stackoverflow.com/questions/25668600/is-1-0-a-valid-output-from-stdgenerate-canonical
///   - https://stackoverflow.com/questions/16224446/stduniform-real-distribution-inclusive-range
///   - https://hal.science/hal-03282794/document
/// @todo Consider fixing the half-open issue for floating point types
template<class T, class Engine>
requires (c_arithmetic<T> || c_value_wrapper<T>)
inline
auto roll_uniform(Engine& prng, T min, T max) -> T {
	if constexpr (std::integral<T>) {
		return std::uniform_int_distribution<T>{min, max}(prng);
	}
	else if constexpr (std::floating_point<T>) {
		return std::uniform_real_distribution<T>{min, max}(prng);
	}
	else if constexpr (c_value_wrapper<T>) {
		return T{roll_uniform<typename T::ValueType>(prng, min.value(), max.value())};
	}
	else {
		static_assert(false, "Unsupported type");
	}
}

template<c_arithmetic T, glm::qualifier Q = glm::defaultp, class Engine>
inline
auto roll_uniform(Engine& prng, glm::vec<2, T, Q> min, glm::vec<2, T, Q> max) -> glm::vec<2, T, Q> {
	return {roll_uniform(prng, min.x, max.x), roll_uniform(prng, min.y, max.y)};
}

template<class Interval, class Engine>
inline
auto roll_uniform(Engine& prng, Interval&& interval)
requires requires { { interval.min() } -> std::same_as<decltype(interval.max())>; } {
	return roll_uniform(prng, interval.min(), interval.max());
}

template<std::uniform_random_bit_generator Engine, std::ranges::random_access_range R>
inline
auto roll_elem_uniform(Engine& prng, R&& r) -> std::ranges::range_reference_t<R> {
	using Diff = std::ranges::range_difference_t<R>;
	const auto idx = std::uniform_int_distribution<Diff> {
		Diff{0}, static_cast<Diff>(std::ranges::ssize(r)) - Diff{1}}(prng);
	return *(std::ranges::begin(std::forward<R>(r)) + idx);
}

template<std::uniform_random_bit_generator Engine>
inline
auto flip_coin(Engine& prng, double p = 0.5) -> bool {
	return std::bernoulli_distribution{p}(prng);
}

} // namespace fr
#endif // include guard
