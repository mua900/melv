#include <app/application.hpp>
#include <app/serialize.hpp>

using namespace melv;

struct State
{
	MeshReference triangle = {};
	MeshReference quad = {};
};

bool initialize(void *userdata, Application *app)
{
	State* state = (State*) userdata;
	app->render.clear_color = ColorF(0.1, 0.2, 0.2);

	TransferMemory triangle, quad;

	Vertex vertices[10] = {
		{ 1,     0,     0, 0, 1, 0, 0, 1},
		{ -0.5,  0.866, 0, 0, 1, 0, 0, 1},
		{ -0.5, -0.866, 0, 0, 1, 0, 0, 1},
	};
	u16 indices[10] = {
		0, 1, 2
	};

	MeshData mesh = {};
	mesh.vertices.add_array(vertices, 3);
	mesh.indices.add_array(indices, 3);

	triangle = add_mesh_to_transfer_buffer(app->render, mesh);

	vertices[0] = { 0,   0,   0, 0, 0, 1, 0, 1 };
	vertices[1] = { 0.5, 0,   0, 0, 0, 1, 0, 1 };
	vertices[2] = { 0.5, 0.5, 0, 0, 0, 1, 0, 1 };
	vertices[3] = { 0,   0.5, 0, 0, 0, 1, 0, 1 };

	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;

	mesh.vertices.discard_data();
	mesh.indices.discard_data();
	mesh.vertices.add_array(vertices, 4);
	mesh.indices.add_array(indices, 6);
	quad = add_mesh_to_transfer_buffer(app->render, mesh);

	if (!app->render.get_command_buffer())
	{
		return false;
	}

	if (!app->render.start_copy_pass())
	{
		log_info("Couldn't start copy pass");
		app->render.submit_command_buffer();
		return false;
	}

	state->triangle = add_mesh(app->render, triangle);
	state->quad = add_mesh(app->render, quad);

	app->render.end_copy_pass();
	app->render.submit_command_buffer();

	return true;
}

void draw(void *userdata, Application *app)
{
	State* state = (State*) userdata;

	// @todo
	// melv::draw_mesh(app->render, state->triangle);
	// melv::draw_mesh(app->render, state->quad);
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
		app->render.clear_color.r += 0.1;
		app->render.clear_color.r = fmodf(app->render.clear_color.r, 1.0f);
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

	InitConfiguration conf = melv::get_default_init_configuration();
	conf.gpuDebug = true;

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
