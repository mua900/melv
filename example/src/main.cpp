#include <app/application.hpp>

using namespace melv;

bool initialize(void *userdata, Application *app)
{
	// example of loading assets

	AssetId vtx = get_asset(String("Vertex"), app->catalog);
	AssetId frag = get_asset(String("Fragment"), app->catalog);

	if (!(vtx.is_valid() && frag.is_valid()))
	{
		return false;
	}

	SDL_GPUShader* vertex = app->catalog.get_shader(vtx);
	SDL_GPUShader* fragment = app->catalog.get_shader(frag);

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

	return false;
}

void handleInput(void* userdata, Application* app)
{
	vec2 mouse_pos = app->input.mouse.pos;

	if ((mouse_pos - vec2(100, 100)).magnitude() < 100)
	{
		log_info("Heyyyy");
	}
}

void updateFunc(void *userdata, Application *app)
{
	float *number = (float*) userdata;

	float dt = app->timeInfo.deltaTimeSeconds;
	*number = dt;
}

void fixedUpdate(void *userdata, Application *app)
{
	float *number = (float*) userdata;

	float dt = app->user.update_state->calculateTimeStep() * app->user.update_state->timeScale;

	*number = dt;
}

int main()
{
	melv::Application app;

	float number = 0;

	UpdateState update = {};

	update.fixedUpdate = fixedUpdate;
	update.update = updateFunc;

	app.user.userdata = &number;
	app.user.init = initialize;
	app.user.draw = draw;
	app.user.event = handleEvent;
	app.user.input = handleInput;
	app.user.update_state = &update;

	if (!app.initialize(melv::get_default_init_configuration()))
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
