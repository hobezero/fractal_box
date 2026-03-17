#include "fractal_box/runtime/runtime.hpp"
#include "asteroids/asteroids.hpp"

auto main(int argc, char* argv[]) -> int {
	auto runtime = fr::Runtime{argc, argv};
	runtime.add_preset(aster::AsteroidsPreset{});

	if (auto result = runtime.run(); !result) {
		return 1;
	}
}
