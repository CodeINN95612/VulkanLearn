#include <spdlog/spdlog.h>

#include "HelloVulkan/App.hpp"

int main(int argc, char** argv) {

    spdlog::set_pattern("[thread %t] [%H:%M:%S] [%^%L%$] %v");
    spdlog::set_level(spdlog::level::debug);

    HelloVulkan::App app;

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
}