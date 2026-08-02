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
	AssetId fragTexId = get_asset(String("FragTexture"), app->catalog);

	Shader vertex = app->catalog.get_shader(vertexId);
	Shader fragment = app->catalog.get_shader(fragmentId);
	Shader frag_texture = app->catalog.get_shader(fragTexId);

	if (vertex.numUniformBuffers != 1 ||
		frag_texture.numSamplers != 1 ||
		frag_texture.numStorageTextures != 1
		)
	{
		log_error("Wrong number of uniform buffers");
		return false;
	}

	if (!app->render.set_shaders(vertex.shader, fragment.shader))
	{
		log_error("Couldn't set shaders");
		return false;
	}

	TransferData triangle, quad;

	float scale = 100;

	// define persistent custom geometry

	Vertex vertices[3] = {
		{ 1,     0,     0, 0, 1, 0, 0, 1},
		{ -0.5,  0.866, 0, 0, 1, 0, 0, 1},
		{ -0.5, -0.866, 0, 0, 1, 0, 0, 1},
	};
	u16 indices[3] = {
		0, 1, 2
	};

	for (int i = 0; i < 3; i++) {
		vertices[i].x *= scale;
		vertices[i].y *= scale;
	}

	Vertex qv[4] = {
		{ 0,     0, 0, 0, 0, 1, 0, 1 },
		{ 0.5,   0, 0, 0, 0, 1, 0, 1 },
		{ 0.5, 0.5, 0, 0, 0, 1, 0, 1 },
		{ 0,   0.5, 0, 0, 0, 1, 0, 1 },
	};
	u16 qi[6] = {
		0, 1, 2,
		0, 2, 3
	};

	for (int i = 0; i < 4; i++) {
		qv[i].x *= scale;
		qv[i].y *= scale;
	}

	DArray<MeshData> meshData(2);

	meshData.add(MeshData());
	meshData.add(MeshData());

	meshData[0].vertices.add_array(vertices, 3);
	meshData[0].indices.add_array(indices, 3);

	meshData[1].vertices.add_array(qv, 4);
	meshData[1].indices.add_array(qi, 6);

	TransferData memory = add_to_transfer_buffer(app->render, meshData);

	meshData.reset();

	int vertex_buffer = app->render.allocate_gpu_buffer(GPUBufferVertex, 1024);
	int index_buffer = app->render.allocate_gpu_buffer(GPUBufferIndex, 1024);

	if (vertex_buffer == -1 || index_buffer == -1)
	{
		return false;
	}

	app->render.set_vertex_buffer(vertex_buffer);
	app->render.set_index_buffer(index_buffer);

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

	melv::draw_mesh(app->render, state->references.get(0));
	melv::draw_mesh(app->render, state->references.get(1));

	// draw known per frame geometry
	melv::draw_arc(app->render, vec2(200, 200), 100, 150, 0, CONSTANT_PI, ColorF(1, 0, 1, 0));
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
