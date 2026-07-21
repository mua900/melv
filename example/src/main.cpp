#include <app/application.hpp>

using namespace melv;

bool initialize(void *userdata, Application *app)
{
	AssetId vtx = get_asset(String("Vertex"), app->catalog);
	AssetId frag = get_asset(String("Fragment"), app->catalog);

	if (!(vtx.is_valid() && frag.is_valid()))
	{
		return false;
	}

	SDL_GPUShader* vertex = app->catalog.get_shader(vtx);
	SDL_GPUShader* fragment = app->catalog.get_shader(frag);

	if (!init_gpu_renderer(&app->render, app->window.window, vertex, fragment))
	{
		return false;
	}

	return true;
}

void draw(void *userdata, Application *app)
{
	melv::draw_triangle(app->render, vec2(100, 100), vec2(200, 200), vec2(100, 200), ColorF(0, 1, 1, 1));
}

int main()
{
	melv::Application app;

	app.user.init = initialize;
	app.user.draw = draw;

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
