#ifndef ASTEROIDS_ASTEROIDS_HPP
#define ASTEROIDS_ASTEROIDS_HPP

#include "fractal_box/runtime/runtime.hpp"

namespace aster {

struct AsteroidsPreset {
	static
	void build(fr::Runtime& runtime);
};

} // namespace aster
#endif
