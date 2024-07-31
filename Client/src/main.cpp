#include "App.h"
#include <exception>
#include <spdlog/spdlog.h>
#include <filesystem>

int main(int argc, char** argv)
{
    spdlog::set_pattern("[thread %t] [%H:%M:%S] [%^%L%$] %v");
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("Initilizing Application:");
    spdlog::info("\t-Argc: {}", argc);
    spdlog::info("\t-Exe Path: {}", argv[0]);

    std::filesystem::path currentPath = std::filesystem::current_path();
    spdlog::info("\t-Working Dir: {}", currentPath.string());

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