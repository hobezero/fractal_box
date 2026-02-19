#ifndef FRACTAL_BOX_CORE_ALIGNMENT_HPP
#define FRACTAL_BOX_CORE_ALIGNMENT_HPP

#include <version>
#ifdef __cpp_lib_hardware_interference_size
#	include <new>
#endif

#include "fractal_box/core/int_types.hpp"

namespace fr {

#ifdef __cpp_lib_hardware_interference_size
inline constexpr auto hardware_destructive_interference_size
	= std::hardware_destructive_interference_size;
inline constexpr auto hardware_constructive_interference_size
	= std::hardware_constructive_interference_size;
#else
inline constexpr size_t hardware_destructive_interference_size = 64uz;
inline constexpr size_t hardware_constructive_interference_size = 64uz;
#endif

inline constexpr auto cacheline_size = hardware_constructive_interference_size;

} // namespace fr
#endif
