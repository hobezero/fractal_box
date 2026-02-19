#ifndef FRACTAL_BOX_CORE_HASHING_HASHING_ATTRIBUTES_HPP
#define FRACTAL_BOX_CORE_HASHING_HASHING_ATTRIBUTES_HPP

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

enum class HashableMode: uint8_t {
	OptOut,
	Default = OptOut,
	OptIn,
	/// @brief As a member of another class, type can be hashed as a sequence of bytes
	AsBytes,
	None,
};

struct Hashable {
	explicit FR_FORCE_INLINE constexpr
	operator bool() const noexcept { return value; }

public:
	bool value = true;
};

} // namespace fr
#endif // include guard
