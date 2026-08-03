#include <app/application.hpp>
#include <app/serialize.hpp>

using namespace melv;

struct Player
{
	vec2 position = {};
};

struct State
{
	Player player;
};

bool initialize(void *userdata, Application *app)
{
	return true;
}

void draw(void *userdata, Application *app)
{
	melv::draw_rectangle(app->render, Rectangle(100, 100, 100, 100), ColorF(1, 1, 0));

	if (app->input.gamepad_count > 0)
	{
		auto& gpad = app->input.gamepads[0];

		auto left = app->render.render_size / 2 - vec2(200, 0);
		auto right = app->render.render_size / 2 + vec2(200, 0);
		float rad = 60;
		auto color_back = ColorF(0.2, 0.2, 0.2);
		melv::draw_circle(app->render, left, rad, color_back);
		melv::draw_circle(app->render, right, rad, color_back);

		auto gleft = gpad.get_left();
		auto gright = gpad.get_right();

		vec2 diff_left  = gleft * rad;
		vec2 diff_right = gright * rad;
		auto color_stick = ColorF(0.7, 0.7, 0.2);
		melv::draw_circle(app->render, left + diff_left, rad, color_stick);
		melv::draw_circle(app->render, right + diff_right, rad, color_stick);
	}
}

bool handleEvent(SDL_Event event, void *userdata, Application* app)
{
	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		switch (event.key.scancode)
		{
			case SDL_SCANCODE_ESCAPE:
			{
				app->quit = true;
				return true;
			}
		}
	}
	else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
	{
		ASSERT(app->input.gamepad_count > 0);
		// log_info("Gamepad Button: %s", get_gamepad_button_name(SDL_GamepadButton(event.gbutton.button)));
	}
	else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
	{
		ASSERT(app->input.gamepad_count > 0);
		// log_info("Axis motion: %d", event.gaxis.value);
	}

	return false;
}

void handleInput(void* userdata, Application* app)
{
	vec2 mouse_pos = app->input.mouse.pos;

	if (app->input.gamepad_count > 0)
	{
		auto& gpad = app->input.gamepads[0];
	}
}

void updateFunc(void *userdata, Application *app)
{
	float dt = app->timeInfo.deltaTimeSeconds;
}

void fixedUpdate(void *userdata, Application *app)
{
	State* state = (State*) userdata;

	float dt = app->user.update_state->calculateTimeStep() * app->user.update_state->timeScale;
}

int main()
{
	melv::Application app;
	State state = {};

	UpdateState update = {};

	update.fixedUpdate = fixedUpdate;
	update.update = updateFunc;

	app.user.userdata = &state;
	app.user.init = initialize;
	app.user.draw = draw;
	app.user.event = handleEvent;
	app.user.input = handleInput;
	app.user.update_state = &update;

	auto conf = melv::get_default_init_configuration();
	conf.flags |= SDL_INIT_GAMEPAD;
	if (!app.initialize(conf))
	{
		return 1;
	}

	while (!app.quit)
	{
		app.handle_events();
		app.update();
		app.draw();
	}

	app.cleanup();

	return 0;
}
