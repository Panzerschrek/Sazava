#pragma once
#include <bitset>
#include <string_view>
#include <variant>
#include <vector>


namespace SZV
{

namespace SystemEventTypes
{
	enum class KeyCode : uint16_t
	{
		// Do not set here key names manually.
		Unknown= 0,

		Escape,
		Enter,
		Space,
		Backspace,
		Tab,

		PageUp,
		PageDown,

		Up,
		Down,
		Left,
		Right,
		BackQuote,

		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,

		// Put new keys at back.
		Minus, Equals,
		SquareBrackretLeft, SquareBrackretRight,
		Semicolon, Apostrophe, BackSlash,
		Comma, Period, Slash,

		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

		Pause,

		// Put it last here.
		KeyCount
	};

	enum KeyModifiers : uint16_t
	{
		Shift= 0x01u,
		Control= 0x02u,
		Alt= 0x04u,
		Caps= 0x08u,
	};

	enum class ButtonCode
	{
		Unknown= 0,
		Left, Right, Middle,
		ButtonCount
	};

	struct KeyEvent
	{
		KeyCode key_code;
		uint16_t modifiers;
		bool pressed;
	};

	struct CharInputEvent
	{
		char ch;
	};

	struct MouseKeyEvent
	{
		ButtonCode mouse_button;
		uint16_t x, y;
		bool pressed;
	};

	struct MouseMoveEvent
	{
		int16_t dx, dy;
	};

	struct WheelEvent
	{
		int16_t delta;
	};

	struct QuitEvent
	{};

using SystemEvent=
	std::variant<
		KeyEvent,
		MouseKeyEvent,
		MouseMoveEvent,
		QuitEvent,
		CharInputEvent >;

using SystemEvents= std::vector<SystemEvent>;

using KeyboardState= std::bitset< size_t(KeyCode::KeyCount) >;
using MouseState= std::bitset< size_t(ButtonCode::ButtonCount) >;

struct InputState
{
	KeyboardState keyboard;
	MouseState mouse;
};

std::string_view GetKeyName(SystemEventTypes::KeyCode key_code);
bool KeyCanBeUsedForControl(SystemEventTypes::KeyCode key_code);

} // namespace SystemEventTypes

using SystemEvent= SystemEventTypes::SystemEvent;
using SystemEvents= SystemEventTypes::SystemEvents;
using InputState= SystemEventTypes::InputState;

} // namespace SZV
