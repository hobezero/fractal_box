#ifndef FRACTAL_BOX_RUNTIME_SYSTEM_UTILS_HPP
#define FRACTAL_BOX_RUNTIME_SYSTEM_UTILS_HPP

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/int_types.hpp"

namespace fr {

template<c_enum Enum, size_t Count>
struct InEnumState {
	constexpr
	auto operator()(Enum current_state) const noexcept -> bool {
		for (auto i = 0uz; i < Count; ++i) {
			if (current_state == acceptable_states[i])
				return true;
		}
		return false;
	}

public:
	Enum acceptable_states[Count];
};

template<c_enum Enum>
InEnumState(Enum ) -> InEnumState<Enum, 1>;

template<c_enum Enum, size_t Count>
InEnumState(const Enum (&)[Count]) -> InEnumState<Enum, Count>;

} // namespace fr
#endif // include guard
