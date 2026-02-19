#ifndef FRACTAL_BOX_CORE_COLOR_HPP
#define FRACTAL_BOX_CORE_COLOR_HPP

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace fr {

using Color3 = glm::vec3;
using Color4 = glm::vec4;

inline constexpr auto color_red = Color3{1.f, 0.f, 0.f};
inline constexpr auto color_green = Color3{0.f, 1.f, 0.f};
inline constexpr auto color_blue = Color3{0.f, 0.f, 1.f};

inline constexpr auto color_cyan = Color3{0.f, 1.f, 1.f};
inline constexpr auto color_magenta = Color3{1.f, 0.f, 1.f};
inline constexpr auto color_yellow = Color3{1.f, 1.f, 0.f};

} // namespace fr
#endif // include guard
