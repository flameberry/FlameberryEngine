#include "Flameberry.h"

int main(int argc, char const* argv[])
{
    Flameberry::Application* app = new Flameberry::Application();
    app->Run();
    delete app;
}
