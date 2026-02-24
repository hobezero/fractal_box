#ifndef ASTEROIDS_SETUP_SYSTEMS_HPP
#define ASTEROIDS_SETUP_SYSTEMS_HPP

#include "fractal_box/core/logging.hpp"
#include "fractal_box/platform/input.hpp"
#include "fractal_box/runtime/runtime.hpp"
#include "asteroids/game_types.hpp"
#include "asteroids/resources.hpp"

namespace aster {

struct ActionSetupSystem {
	static
	void run(fr::Input& input) {
		input.map_to_action(fr::ScanCode::A, fr::SwitchEvent::Down, action_turn_left);
		input.map_to_action(fr::ScanCode::D, fr::SwitchEvent::Down, action_turn_right);
		input.map_to_action(fr::ScanCode::W, fr::SwitchEvent::Down, action_engine_burn);
		input.map_to_action(fr::ScanCode::S, fr::SwitchEvent::Down, action_engine_reverse_burn);
		input.map_to_action(fr::ScanCode::Space, fr::SwitchEvent::Down, action_fire);

		input.map_to_action(fr::ScanCode::Left, fr::SwitchEvent::Down, action_turn_left);
		input.map_to_action(fr::ScanCode::Right, fr::SwitchEvent::Down, action_turn_right);
		input.map_to_action(fr::ScanCode::Up, fr::SwitchEvent::Down, action_engine_burn);
		input.map_to_action(fr::ScanCode::Down, fr::SwitchEvent::Down, action_engine_reverse_burn);

		input.map_to_action(fr::ScanCode::P, fr::SwitchEvent::JustPressed, action_toggle_pause);
		input.map_to_action(fr::ScanCode::Return, fr::SwitchEvent::JustReleased, action_accept);
	}
};

struct ResourceLoadingSystem {
	static
	auto run(fr::Runtime& runtime) -> fr::ErrorOr<> {
		auto rsc = GameResources{};
		auto result = rsc.init();
		runtime.add_part(std::move(rsc));
		return result;
	}
};

struct AppSetupSystem {
	static
	void run() { }
};

struct GameSetupSystem {
	static
	void run(fr::MessageWriter<ReqGameStart>& writer) {
		FR_LOG_INFO_MSG("Sending StartGameMsg");
		writer.push(ReqGameStart{});
	}
};

} // namespace aster
#endif // include guard
