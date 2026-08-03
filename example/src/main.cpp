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
		if (app->input.gamepad_count > 0)
		{
			GamepadState& gpad = app->input.gamepads[0];
			log_info("South: %d", gpad.south());
			log_info("North: %d", gpad.north());
			log_info("West: %d", gpad.west());
			log_info("East: %d", gpad.east());
		}
	}

	return false;
}

void handleInput(void* userdata, Application* app)
{
	vec2 mouse_pos = app->input.mouse.pos;
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
