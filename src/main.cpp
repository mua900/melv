#include "app/application.hpp"

int main()
{
    Application app;

    if (!app.initialize())
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
