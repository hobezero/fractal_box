#ifndef FRACTAL_BOX_PLATFORM_INPUT_HPP
#define FRACTAL_BOX_PLATFORM_INPUT_HPP

/// @todo
///   TODO: https://gamedev.net/blogs/entry/2250186-designing-a-robust-input-handling-system-for-games/

#include <algorithm>
#include <optional>
#include <vector>

#include <SDL2/SDL_events.h>
#include <glm/vec2.hpp>

#include "fractal_box/core/containers/linear_flat_set.hpp"
#include "fractal_box/core/containers/unordered_map.hpp"
#include "fractal_box/core/hashing/hashed_string.hpp"
#include "fractal_box/core/hashing/hashing_attributes.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/description_types.hpp"
#include "fractal_box/core/preprocessor.hpp"
#include "fractal_box/platform/input_num_codes.hpp"

namespace fr {

struct KeyEvent {
	KeyCode key = KeyCode::Unknown;
	KeyModifiers modifiers = {};
	bool is_repeated = false;
};

struct MouseClickEvent {
	MouseButton button;
	glm::ivec2 pos;
	int click_count;
	KeyModifiers modifiers;
};

struct MouseMoveEvent {
	glm::ivec2 pos;
	glm::ivec2 relative_pos;
	MouseButtonFlags buttons;
	KeyModifiers modifiers;
};

struct MouseScrollEvent {
	glm::vec2 offset;
	std::optional<glm::ivec2> pos;
	KeyModifiers modifers;
};

enum class SwitchState: uint8_t {
	Up,
	Down
};

template<c_enum Switch, size_t MaxCacheSize = 1024>
requires (c_enum<Switch> || std::integral<Switch>)
class CachedSwitchSet {
public:
	CachedSwitchSet() = default;
	explicit CachedSwitchSet(size_t reserved_size): _cache(reserved_size) { }

	void clear() noexcept {
		static_assert(noexcept(_pressed.clear()));
		std::fill(_cache.begin(), _cache.end(), SwitchState::Up);
		_pressed.clear();
	}

	auto get(Switch key) const noexcept -> SwitchState {
		// Return from the flat map if within limit, otherwise look into the world map for
		// the 'unusual' keys
		const auto key_int = to_underlying(key);
		const auto index = static_cast<size_t>(key_int);
		if (0 <= key_int && index < MaxCacheSize) {
			if (index < _cache.size())
				return _cache[index];
			else
				return SwitchState::Up;
		}
		return _pressed.contains(key) ? SwitchState::Down : SwitchState::Up;
	}

	void set(Switch key, SwitchState value) {
		const auto key_int = to_underlying(key);
		const auto index = static_cast<size_t>(key_int);
		if (0 <= key_int && index < MaxCacheSize) {
			if (index >= _cache.size()) {
				_cache.resize(index + 1);
			}
			_cache[index] = value;
		}

		if (value == SwitchState::Down) {
			_pressed.insert(key);
		}
		else {
			_pressed.erase(key);
		}
	}

	auto is_down(Switch key) const noexcept -> bool { return get(key) == SwitchState::Down; }
	auto is_up(Switch key) const noexcept -> bool { return get(key) == SwitchState::Up; }

	void set_down(Switch key) { set(key, SwitchState::Down); }
	void set_up(Switch key) { set(key, SwitchState::Up); }

	auto begin() const noexcept { return _pressed.cbegin(); }
	auto cbegin() const noexcept { return _pressed.cbegin(); }
	auto end() const noexcept { return _pressed.cend(); }
	auto cend() const noexcept { return _pressed.cend(); }

	auto size() const noexcept { return _pressed.size(); }
	[[nodiscard]] auto empty() const noexcept -> bool { return _pressed.empty(); }

protected:
	/// @brief State of the first 1024 switches
	/// Not a `std::vector<bool>` because of the specialization idiocy
	/// @todo TODO: Replace with std::bitset or equivalent
	std::vector<SwitchState> _cache;

	/// @brief All of the keys, including with switch index >= 1024 (mostly Unicode)
	LinearFlatSet<Switch> _pressed;
};

/// @brief Holds the state of each key of the virtual keyboard
class KeyboardState: public CachedSwitchSet<ScanCode, num_scancodes> {
	using Base = CachedSwitchSet;
public:
	explicit KeyboardState(size_t reservedSize = 128);
};

/// @brief Holds the state of each key of the virtual mouse, as well as cursor position
/// (absolute and relative) and scroll offset
class MouseState: public CachedSwitchSet<MouseButton, max_num_mouse_buttons> {
	using Base = CachedSwitchSet;
public:
	explicit MouseState(size_t reserved_size = 16);

	void clear() noexcept;

	auto was_moved() const noexcept -> bool { return _relativePosition != glm::ivec2{}; }
	auto was_scrolled() const noexcept -> bool { return _scrollOffset != glm::vec2{}; }

	auto position() const noexcept -> glm::ivec2 { return _position; }
	void set_position(glm::ivec2 position) noexcept { _position = position; }

	auto relative_position() const noexcept -> glm::ivec2 { return _relativePosition; }
	void set_relative_position(glm::ivec2 position) noexcept { _relativePosition = position; }

	auto scroll_offset() const noexcept -> glm::vec2 { return _scrollOffset; }
	void set_scroll_offset(glm::vec2 offset) noexcept { _scrollOffset = offset; }

private:
	/// @brief Cursor position in window pixel coordinates
	glm::ivec2 _position {};
	/// @brief Change in cursor position in window pixel coordinates
	glm::ivec2 _relativePosition {};
	glm::vec2 _scrollOffset {};
};

/// @brief Generic representation of states and state transitions of binary switches
/// (keys and buttons)
enum class SwitchEvent: uint8_t {
//	/// @brief Switch is in OFF state. Fired every frame
//	Up,
	/// @brief The switch is in ON state. Fired every frame the button is down
	Down,
	/// @brief The switch has just changed its state from OFF to ON. Fired once
	JustPressed,
	/// @brief The switch has just changed its state from ON to OFF. Fired once
	JustReleased,
};

/// @brief Implements Godot-like API for mapping and accessing user input actions
///
/// The biggest difference between our fr::Input and Godot Input is that we can map
/// a specific state-state transition of a button to the action. Consider a situation when multiple
/// buttons are assigned to the action and only one is released while others are still pressed,
/// which action should be activated? By allowing to specify the action mode while mapping
/// we avoid ambiguous behavior.
class Input {
public:
	/// Actions: switches and axes
	/// TODO: Axes
	using ActionId = StringId;

	static constexpr
	auto make_action(std::string_view name) noexcept -> ActionId {
		return {name.data(), name.size()};
	}

	void map_to_action(ScanCode key, SwitchEvent mode, ActionId action_id);
	void unmap(ScanCode key, SwitchEvent mode);
	void map_to_action(MouseButton button, SwitchEvent mode, ActionId action_id);
	void unmap(MouseButton button, SwitchEvent mode);

	/// @returns `true` if the event was handled, `false` otherwise
	auto accept_event(const SDL_Event &event) -> bool;
	void accept_key_press_event(ScanCode key);
	void accept_key_release_event(ScanCode key);
	void accept_mouse_press_event(MouseButton button, glm::ivec2 position);
	void accept_mouse_release_event(MouseButton button, glm::ivec2 position);
	void accept_mouse_move_event(glm::ivec2 position, glm::ivec2 relative_position);
	void accept_mouse_scroll_event(glm::vec2 scroll_offset);

	void release_keys() noexcept;
	void release_mouse() noexcept;
	void releaseKeysAndMouse() noexcept {
		release_keys();
		release_mouse();
	}

	void emit_down_actions();

	auto get_action(ActionId action_id) const -> bool;
	auto get_action_strength(ActionId action_id) const -> float;
	auto all_actions() const noexcept -> const auto& { return _action_set; }

	void clear_actions();

	auto keyboard() const noexcept -> const KeyboardState& { return _keyboard; }
	auto mouse() const noexcept -> const MouseState& { return _mouse; }

private:
	/// @brief PIMPL without data
	struct Impl;

	using SwitchId = uint64_t;
	static_assert(sizeof(SwitchId) >= sizeof(KeyCode) + sizeof(MouseButton));

	struct SwitchInput {
		SwitchId id;
		SwitchEvent mode;
		// TODO: modifiers?

		friend consteval
		auto kepler_describe(SwitchInput) noexcept {
			return class_desc<
				Attributes<Hashable{}>,
				Field<&SwitchInput::id>,
				Field<&SwitchInput::mode>
			>;
		}

		auto operator==(const SwitchInput& rhs) const noexcept -> bool = default;
	};

	// std::vector because not many actions get fired each frame
	using ActionSet = std::vector<ActionId>;
	// At some point will be replaced with something better
	using InputToActionMap = UnorderedMap<SwitchInput, ActionId>;

	ActionSet _action_set;
	InputToActionMap _input_map;

	KeyboardState _keyboard;
	MouseState _mouse;
};

inline namespace literals {

inline constexpr
auto operator ""_action(const char* str, std::size_t size) noexcept -> Input::ActionId {
	return {str, size};
}

} // namespace literals

} // namespace fr

#define FR_DEFINE_INPUT_ACTION_ID(name) \
	inline constexpr ::fr::Input::ActionId name = FR_TO_STRING(name)

#endif // include guard
