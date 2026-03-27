#ifndef FRACTAL_BOX_CORE_SERIALIZATION_SERIALIZATION_ATTRIBUTES_HPP
#define FRACTAL_BOX_CORE_SERIALIZATION_SERIALIZATION_ATTRIBUTES_HPP

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

enum class SerializableMode: uint8_t {
	OptOut,
	OptIn,
	None,

	Default = OptOut
};

struct Serializable {
	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return value; }

public:
	bool value = true;
};

} // namespace fr
#endif // include guard
