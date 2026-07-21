#include <app/application.hpp>

using namespace melv;

void draw(void *userdata, Application *app)
{
	melv::draw_triangle(app->render, vec2(100, 100), vec2(200, 200), vec2(100, 200), ColorF(0, 1, 1, 1));
}

int main()
{
	melv::Application app;

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
