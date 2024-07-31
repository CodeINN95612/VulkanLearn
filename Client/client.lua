project "Client"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir (OutDir)
    objdir (ObjDir)
    debugdir "%{wks.location}"

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "src",
        "%{wks.location}/Renderer/src",
        "%{wks.location}/dependencies/spdlog/spdlog/include",
        "%{wks.location}/dependencies/glm/glm",
        "%{wks.location}/dependencies/imgui",
        "%{wks.location}/dependencies/imgui/imgui",
        "%{wks.location}/dependencies/glfw/glfw/include",
        "%{vulkanSDK}/Include"
    }

    libdirs
    {
        "%{vulkanSDK}/Lib"
    }

    links
    {
        "Renderer",
        "imgui",
        "glfw",
        "vulkan-1"
    }

    filter "action:vs*"
        flags { "MultiProcessorCompile" }