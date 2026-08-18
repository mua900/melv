#include <app/application.hpp>
#include <app/serialize.hpp>

#include <random>
#include <time.h>

using namespace melv;

struct State
{
	float number = 0;
	DArray<MeshReference> references = {};
	TextureHandle texture = {};
	DrawGroupId group = {};

	vec2 position[64];
};

#define CHANGE_SHADERS 1

bool initialize(void *userdata, Application *app)
{
	srand(time(0));

	State* state = (State*) userdata;
	app->render.clear_color = Colorf(0.1, 0.2, 0.2);

	for (int i = 0; i < 64; i++)
	{
		int range = 600;
		float x = rand() % range;
		float y = rand() % range;

		x = x - range / 2;
		y = y - range / 2;

		state->position[i] = vec2(x, y);
	}

	AssetId vertexId = get_asset(String("Vertex"), app->catalog);
	AssetId vertexInstId = get_asset(String("VertexInstance"), app->catalog);
	AssetId fragmentId = get_asset(String("Fragment"), app->catalog);
	AssetId fragmentTexId = get_asset(String("FragTexture"), app->catalog);
	AssetId texId = get_asset(String("TestImg"), app->catalog);

	Shader vertex = app->catalog.get_shader(vertexId);
	Shader fragment = app->catalog.get_shader(fragmentId);
	Shader vertexInst = app->catalog.get_shader(vertexInstId);
	Shader fragmentTex = app->catalog.get_shader(fragmentTexId);

	TextureHandle texture = app->catalog.get_image(texId);

	ASSERT(texture != TEXTURE_HANDLE_INVALID);

#if CHANGE_SHADERS
	if (!app->render.set_shaders(&app->render.graphics, vertex.shader, fragment.shader))
	{
		return false;
	}

	if (!app->render.set_shaders(&app->render.graphics_texture, vertex.shader, fragmentTex.shader))
	{
		return false;
	}

	if (!app->render.set_shaders(&app->render.graphics_instance_texture, vertexInst.shader, fragmentTex.shader))
	{
		return false;
	}
#endif

	state->group = app->render.make_draw_group(texture, 1024);
	if (state->group == DRAW_GROUPID_INVALID)
	{
		return false;
	}

	melv::DrawGroupId id = app->render.make_draw_group(texture, 1024 * 4);

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
		{ 0,     0, 0, 1, 1, 1, 1, 1 },
		{ 0.5,   0, 1, 1, 1, 1, 1, 1 },
		{ 0.5, 0.5, 1, 0, 1, 1, 1, 1 },
		{ 0,   0.5, 0, 0, 1, 1, 1, 1 },
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

	app->active_camera = init_camera();
	app->render.camera = &app->active_camera;

	return true;
}

void draw(void *userdata, Application *app)
{
	State* state = (State*) userdata;

	MeshDraw draw = {};
	draw.mesh = state->references.get(0);

	melv::queue_draw_mesh(app->render, draw);

	draw.mesh = state->references.get(1);
	draw.texture = state->texture;
	melv::queue_draw_mesh(app->render, draw);

	vec2 offset = vec2(0.5, 0.2);
	vec2 scale = vec2(1,1);

	InstanceData q = {};
	q.sourceOffset = pack_unorm16x2(offset);
	q.sourceScale = pack_unorm16x2(scale);

	q.x = 100;
	q.y = 100;
	q.rotation = 0;
	q.scalex = 100;
	q.scaley = 100;
	q.color = 0xFFFFFFFF;

	melv::queue_draw_group(app->render, q, state->group);
	q.y += 200;
	melv::queue_draw_group(app->render, q, state->group);
	q.x += 200;
	q.y += 150;
	melv::queue_draw_group(app->render, q, state->group);
	q.x += 200;
	q.color = melv::colorToHex(Colorf(1,0,0));
	melv::queue_draw_group(app->render, q, state->group);
	q.color = 0xffffffff;

	for (int i = 0; i < 1; i++)
	{
		q.x = state->position[i].x;
		q.y = state->position[i].y;
		melv::queue_draw_group(app->render, q, state->group);
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
			case SDL_SCANCODE_E:
			{
				app->active_camera.zoom -= 0.1;
				return true;
			}
			case SDL_SCANCODE_Q:
			{
				app->active_camera.zoom += 0.1;
				return true;
			}
		}
	}
	else if (event.type == SDL_EVENT_MOUSE_WHEEL)
	{
		float zoom = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
		{
			zoom = -zoom;
		}

		app->active_camera.zoom += zoom * 0.1f;

		app->active_camera.zoom = melv::clamp(0.1, 10, app->active_camera.zoom);
	}

	return false;
}

void handleInput(void* userdata, Application* app)
{
	vec2 mouse_pos = app->input.mouse.pos;

	if ((vec2(100, 100) - mouse_pos).magnitude() < 100)
	{
		app->render.clear_color.r += 0.01;
		app->render.clear_color.r = fmodf(app->render.clear_color.r, 0.8f);

		app->active_camera.position.x += 0.8 * app->active_camera.zoom;
		app->active_camera.zoom += 0.02;
		if (app->active_camera.zoom > 2)
		{
			app->active_camera.zoom = 1;
		}
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
	conf.render.gpuDebug = true;

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
