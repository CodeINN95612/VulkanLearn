#include "App.h"
#include <exception>
#include <spdlog/spdlog.h>

int main(int argc, char** argv)
{
    spdlog::set_pattern("[thread %t] [%H:%M:%S] [%^%L%$] %v");
    spdlog::set_level(spdlog::level::debug);

    App app;
    try
    {
        app.Run();
    }
    catch (const std::exception& e)
    {
        spdlog::critical(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;


	return 0;
}