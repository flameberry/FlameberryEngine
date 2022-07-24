#include "Flameberry.h"

int main(int argc, char const* argv[])
{
    FL_LOGGER_INIT();
    FL_INFO("Initialized Logger!");
    Flameberry::Application app;
    app.Run();
}
