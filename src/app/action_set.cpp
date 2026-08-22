#include "action_set.hpp"

namespace melv {

	void ActionSet::add_action(const char* name, Input& input)
	{
		String s = String(name);
		Action* action = action_set.find(s);
		if (!action)
		{
			Action naction = {};
			naction.add_input(input);
			action_set.add(s, naction);
		}
		else
		{
			action->add_input(input);
		}
	}

	bool ActionSet::read_action(InputState& state, const char* name, Value& out)
	{
		String s = String(name);
		Action* action = action_set.find(s);
		if (!action)
		{
			return false;
		}

		return action->read_value(state, out);
	}

	void Action::add_input(Input& in)
	{
		if (num_input < ACTION_MAX_INPUT)
		{
			input[num_input] = in;
			num_input += 1;
		}
	}

	bool Action::read_value(InputState& state, Value& out)
	{
		bool read = false;
		int i = 0;
		while ((!read) && i < num_input)
		{
			switch (input[i].kind)
			{
				case InputMouseButton:
				{
					SDL_MouseButtonFlags flags = (SDL_MouseButtonFlags)input[i].key;
					bool down = flags & state.mouse.buttonFlags;
					out = Value(down);
					return true;
				}
				case InputMouseWheel:
				{
					vec2 wheel = state.mouse.wheel;
					out = Value(wheel);
					return true;
				}
				case InputKeyboardKey:
				{
					if (input[i].key >= state.keyboard.num_keys)
					{
						return false;
					}

					bool down = state.keyboard.keys[input[i].key];
					out = Value(down);
					return true;
				}
				case InputGamepadButton:
				{
					auto button = (SDL_GamepadButton)input[i].key;
					bool down = state.gamepads[input[i].device].get_button(button);
					out = Value(down);
					return true;
				}
				case InputGamepadAxis:
				{
					auto gamepad_axis = (SDL_GamepadAxis) input[i].key;
					float axis = state.gamepads[input[i].device].get_axis(gamepad_axis);
					out = Value(axis);
					return true;
				}
				case InputGamepadVector:
				{
					vec2 v = {};
					if (input[i].key == GamepadVector_Left)
					{
						v = state.gamepads[input[i].device].get_left();
					}
					else if (input[i].key == GamepadVector_Right)
					{
						v = state.gamepads[input[i].device].get_right();
					}
					else if (input[i].key == GamepadVector_Triggers)
					{
						v = state.gamepads[input[i].device].get_triggers();
					}
					else return false;

					out = Value(v);

					return true;
				}
			}

			i += 1;
		}

		return false;
	}

} // namespace
