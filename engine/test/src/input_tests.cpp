#include "fractal_box/platform/input.hpp"

#include <type_traits>

#include <catch2/catch_test_macros.hpp>

static constexpr auto largeKey = static_cast<fr::ScanCode>(fr::num_scancodes - 1);

TEST_CASE("KeyboardState", "[u][engine][core][input]") {
	auto keyboard = fr::KeyboardState{};

	SECTION("rule-of-six") {
		CHECK(std::is_default_constructible_v<fr::KeyboardState>);
		CHECK(std::is_copy_constructible_v<fr::KeyboardState>);
		CHECK(std::is_copy_assignable_v<fr::KeyboardState>);
		CHECK(std::is_move_constructible_v<fr::KeyboardState>);
		CHECK(std::is_nothrow_move_constructible_v<fr::KeyboardState>);
		CHECK(std::is_move_assignable_v<fr::KeyboardState>);
		CHECK(std::is_nothrow_move_assignable_v<fr::KeyboardState>);
		CHECK(std::is_destructible_v<fr::KeyboardState>);
	}
	SECTION("initialized state is empty") {
		CHECK(keyboard.is_up(fr::ScanCode::A));
		CHECK(keyboard.is_up(fr::ScanCode::S));
		CHECK(keyboard.is_up(fr::ScanCode::Comma));
		CHECK(keyboard.is_up(fr::ScanCode::LAlt));
		CHECK(keyboard.get(fr::ScanCode::Digit9) == fr::SwitchState::Up);
	}
	SECTION("press a few keys") {
		keyboard.set_down(fr::ScanCode::S);
		keyboard.set_down(fr::ScanCode::Backspace);
		keyboard.set_down(largeKey);
		keyboard.set(fr::ScanCode::RShift, fr::SwitchState::Down);

		SECTION("only pressed keys are down") {
			CHECK(keyboard.is_down(fr::ScanCode::S));
			CHECK(keyboard.is_down(fr::ScanCode::Backspace));
			CHECK(keyboard.is_down(largeKey));
			CHECK(keyboard.get(fr::ScanCode::RShift) == fr::SwitchState::Down);
			CHECK(keyboard.is_up(fr::ScanCode::A));
			CHECK(keyboard.is_up(fr::ScanCode::Return));
		}
		SECTION("release keys") {
			keyboard.set_up(fr::ScanCode::Backspace);
			CHECK(keyboard.is_up(fr::ScanCode::Backspace));
			CHECK(keyboard.is_down(fr::ScanCode::S));

			keyboard.set_up(largeKey);
			CHECK(keyboard.is_up(largeKey));
			CHECK(keyboard.is_down(fr::ScanCode::S));
		}
	}
}

TEST_CASE("Input", "[u][engine][core][input]") {
	fr::Input input;
	constexpr fr::Input::ActionId action_a{fr::adopt, 10};
	constexpr fr::Input::ActionId action_b{fr::adopt, 123'456'789};

	SECTION("rule-of-six") {
		CHECK(std::is_default_constructible_v<fr::Input>);
		CHECK(std::is_copy_constructible_v<fr::Input>);
		CHECK(std::is_copy_assignable_v<fr::Input>);
		CHECK(std::is_move_constructible_v<fr::Input>);
		CHECK(std::is_nothrow_move_constructible_v<fr::Input>);
		CHECK(std::is_move_assignable_v<fr::Input>);
		CHECK(std::is_nothrow_move_assignable_v<fr::Input>);
		CHECK(std::is_destructible_v<fr::Input>);
	}
	SECTION("initialized state is empty") {
		CHECK(!input.get_action(action_a));
		CHECK(!input.get_action(action_b));
		CHECK(input.keyboard().empty());
		CHECK(input.mouse().empty());
	}
	SECTION("press button") {
		[[maybe_unused]] constexpr fr::Input::ActionId just_released{fr::adopt, 11};
		[[maybe_unused]] constexpr fr::Input::ActionId down{fr::adopt, 123'456'789};
		[[maybe_unused]] constexpr fr::Input::ActionId action_c{fr::adopt, 567};

		input.accept_key_press_event(fr::ScanCode::D);
		CHECK(input.all_actions().empty());

		input.map_to_action(fr::ScanCode::F, fr::SwitchEvent::JustPressed, action_a);
		input.map_to_action(fr::ScanCode::F, fr::SwitchEvent::JustReleased, just_released);
		input.map_to_action(fr::ScanCode::U, fr::SwitchEvent::JustPressed, action_a);
		input.map_to_action(fr::ScanCode::F, fr::SwitchEvent::Down, down);

		CHECK(!input.get_action(action_a));
		CHECK(input.all_actions().empty());
		input.accept_key_press_event(fr::ScanCode::F);
		CHECK(input.get_action(action_a));

		CHECK(!input.get_action(down));
		input.emit_down_actions();
		CHECK(input.get_action(down));

		CHECK(!input.get_action(just_released));
		input.accept_key_release_event(fr::ScanCode::F);
		CHECK(input.get_action(just_released));

		SECTION("clear_actions() clears actions") {
			input.clear_actions();
			CHECK(input.all_actions().empty());
		}
	}
}
