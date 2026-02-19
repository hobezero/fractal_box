#include "fractal_box/platform/input.hpp"

#include <algorithm>

namespace fr {

using KeyInt = std::underlying_type_t<KeyCode>;

KeyboardState::KeyboardState(size_t reservedSize)
	: Base(reservedSize)
{ }

MouseState::MouseState(size_t reserved_size)
	: Base(reserved_size)
{ }

void MouseState::clear() noexcept {
	Base::clear();
	_position = {};
	_relativePosition = {};
	_scrollOffset = {};
}

struct Input::Impl {
	static constexpr
	auto to_switch_id(ScanCode key) noexcept {
		static_assert(sizeof(SwitchId) >= sizeof(ScanCode));
		// ScanCode goes to the lower bits
		return static_cast<SwitchId>(key);
	}

	static constexpr
	auto to_switch_id(MouseButton btn) noexcept {
		static_assert(sizeof(SwitchId) >= sizeof(MouseButton));
		// Shift to the higher bits
		constexpr auto shift = sizeof(SwitchId) - sizeof(MouseButton);
		return static_cast<SwitchId>(btn) << shift;
	}
};

void Input::map_to_action(ScanCode key, SwitchEvent mode, ActionId action_id) {
	const auto id = Impl::to_switch_id(key);
	_input_map.insert_or_assign(SwitchInput{id, mode}, action_id);
}

void Input::unmap(ScanCode key, SwitchEvent mode) {
	const auto id = Impl::to_switch_id(key);
	const auto pos = _input_map.find(SwitchInput{id, mode});
	if (pos != _input_map.end()) {
		_input_map.erase(pos);
	}
}

void Input::map_to_action(MouseButton button, SwitchEvent mode, ActionId action_id) {
	_input_map.insert_or_assign(SwitchInput{Impl::to_switch_id(button), mode}, action_id);
}

void Input::unmap(MouseButton button, SwitchEvent mode) {
	const auto pos = _input_map.find({Impl::to_switch_id(button), mode});
	if (pos != _input_map.end()) {
		_input_map.erase(pos);
	}
}

auto Input::accept_event(const SDL_Event &event) -> bool {
	switch (event.type) {
		case SDL_KEYDOWN: {
			accept_key_press_event(static_cast<ScanCode>(event.key.keysym.scancode));
			return true;
		}
		case SDL_KEYUP: {
			accept_key_release_event(static_cast<ScanCode>(event.key.keysym.scancode));
			return true;
		}
		case SDL_MOUSEMOTION: {
			const auto& e = event.motion;
			accept_mouse_move_event({e.x, e.y}, {e.xrel, e.yrel});
			return true;
		}
		case SDL_MOUSEBUTTONDOWN: {
			const auto& e = event.button;
			accept_mouse_press_event(static_cast<MouseButton>(e.button), {e.x, e.y});
			return true;
		}
		case SDL_MOUSEBUTTONUP: {
			const auto& e = event.button;
			accept_mouse_release_event(static_cast<MouseButton>(e.button), {e.x, e.y});
			return true;
		}
		case SDL_MOUSEWHEEL: {
			const auto& e = event.wheel;
			accept_mouse_scroll_event({e.x, e.y});
			return true;
		}
		default:
			return false;
	}
}

void Input::accept_key_press_event(ScanCode key) {
	if (_keyboard.is_up(key)) {
		const auto input = SwitchInput{Impl::to_switch_id(key), SwitchEvent::JustPressed};
		if (const auto action_it = _input_map.find(input); action_it != _input_map.end()) {
			_action_set.push_back(action_it->second);
		}
		_keyboard.set_down(key);
	}
}

void Input::accept_key_release_event(ScanCode key) {
	if (_keyboard.is_down(key)) {
		const auto input = SwitchInput{Impl::to_switch_id(key), SwitchEvent::JustReleased};
		if (const auto action_it = _input_map.find(input); action_it != _input_map.end()) {
			_action_set.push_back(action_it->second);
		}
		_keyboard.set_up(key);
	}
}

void Input::accept_mouse_press_event(MouseButton button, glm::ivec2 position) {
	if (_mouse.is_up(button)) {
		const auto input = SwitchInput{Impl::to_switch_id(button), SwitchEvent::JustPressed};
		if (const auto action_it = _input_map.find(input); action_it != _input_map.end()) {
			_action_set.push_back(action_it->second);
		}
		_mouse.set_down(button);
	}
	_mouse.set_position(position);
}

void Input::accept_mouse_release_event(MouseButton button, glm::ivec2 position) {
	if (_mouse.is_down(button)) {
		const auto input = SwitchInput{Impl::to_switch_id(button), SwitchEvent::JustReleased};
		if (const auto action_it = _input_map.find(input); action_it != _input_map.end()) {
			_action_set.push_back(action_it->second);
		}
		_mouse.set_up(button);
	}
	_mouse.set_position(position);
}

void Input::accept_mouse_move_event(glm::ivec2 position, glm::ivec2 relative_position) {
	_mouse.set_position(position);
	_mouse.set_relative_position(relative_position);
}

void Input::accept_mouse_scroll_event(glm::vec2 scroll_offset) {
	_mouse.set_scroll_offset(scroll_offset);
}

void Input::release_keys() noexcept {
	_keyboard.clear();
}

void Input::release_mouse() noexcept {
	_mouse.clear();
}

void Input::emit_down_actions() {
	const auto process = [this](const auto& container) {
		// For each pressed key, create an action if Down mode was mapped
		for (const auto key : container) {
			const auto id = Impl::to_switch_id(key);
			if (const auto it = _input_map.find({id, SwitchEvent::Down}); it != _input_map.end()) {
				_action_set.push_back(it->second);
			}
		}
	};
	process(_keyboard);
	process(_mouse);
}

auto Input::get_action(ActionId action_id) const -> bool {
	const auto pos = std::find(_action_set.cbegin(), _action_set.cend(), action_id);
	return pos != _action_set.cend();
}

auto Input::get_action_strength(ActionId action_id) const -> float {
	FR_UNUSED(action_id);
	// TODO: implement axes
	return NAN;
}

void Input::clear_actions() {
	_action_set.clear();
}

} // namespace fr
