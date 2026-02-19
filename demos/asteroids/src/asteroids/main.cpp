#include "fractal_box/core/logging.hpp"
#include "fractal_box/runtime/runtime.hpp"
#include "asteroids/asteroids.hpp"

auto main(int argc, char* argv[]) -> int {
	auto runtime = fr::Runtime{argc, argv};
	runtime.add_preset(aster::AsteroidsPreset{});

	if (auto result = runtime.run(); !result) {
		FR_LOG_FATAL("{}", result.error());
		return to_return_code(result.error().code());
	}
}
