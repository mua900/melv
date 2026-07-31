#include <app/application.hpp>
#include <app/serialize.hpp>

using namespace melv;

struct State
{
	float number = 0;
	DArray<MeshReference> references = {};
};

bool initialize(void *userdata, Application *app)
{
	State* state = (State*) userdata;
	app->render.clear_color = ColorF(0.1, 0.2, 0.2);

	AssetId vertexId = get_asset(String("Vertex"), app->catalog);
	AssetId fragmentId = get_asset(String("Fragment"), app->catalog);

	SDL_GPUShader *vertex = app->catalog.get_shader(vertexId);
	SDL_GPUShader *fragment = app->catalog.get_shader(fragmentId);

	if (!app->render.set_shaders(vertex, fragment))
	{
		log_error("Couldn't set shaders");
		// return false;
	}

	TransferData triangle, quad;

	Vertex vertices[10] = {
		{ 1,     0,     0, 0, 1, 0, 0, 1},
		{ -0.5,  0.866, 0, 0, 1, 0, 0, 1},
		{ -0.5, -0.866, 0, 0, 1, 0, 0, 1},
	};
	u16 indices[10] = {
		0, 1, 2
	};

	DArray<MeshData> meshData(2);

	meshData.add(MeshData());
	meshData.add(MeshData());

	meshData[0].vertices.add_array(vertices, 3);
	meshData[0].indices.add_array(indices, 3);

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

	meshData[1].vertices.add_array(vertices, 4);
	meshData[1].indices.add_array(indices, 6);

	TransferData memory = add_to_transfer_buffer(app->render, meshData);

	meshData.reset();

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

	state->references = upload_mesh_data(app->render, memory);

	app->render.end_copy_pass();
	app->render.submit_command_buffer();

	return true;
}

void draw(void *userdata, Application *app)
{
	State* state = (State*) userdata;

	// @todo
	melv::draw_mesh(app->render, state->references.get(0));
	melv::draw_mesh(app->render, state->references.get(1));
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
	State *state = (State*) userdata;

	float dt = app->timeInfo.deltaTimeSeconds;
	state->number = dt;
}

void fixedUpdate(void *userdata, Application *app)
{
	State *state = (State*) userdata;

	float dt = app->user.update_state->calculateTimeStep() * app->user.update_state->timeScale;

	state->number = dt;
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
