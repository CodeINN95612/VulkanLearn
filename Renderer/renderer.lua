project "Renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir (OutDir)
    objdir (ObjDir)

    files
    {
        "include/**.h",
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "src",
        "%{wks.location}/dependencies/spdlog/spdlog/include",
        "%{wks.location}/dependencies/glm/glm",
        "%{wks.location}/dependencies/fastgltf/fastgltf/include",
        "%{wks.location}/dependencies/imgui",
        "%{wks.location}/dependencies/imgui/imgui",
        "%{wks.location}/dependencies/glfw/glfw/include",
        "%{wks.location}/dependencies/vk-boostrap/vk-boostrap/src",
        "%{vulkanSDK}/Include"
    }

    libdirs
    {
        "%{vulkanSDK}/Lib"
    }

    links
    {
        "vulkan-1",
        "fastgltf",
        "glfw",
        "imgui",
        "vk-boostrap"
    }

    filter "configurations:Debug"
        links
        {
            "shadercd",
            "shaderc_utild",
            "shaderc_sharedd",
            "shaderc_combinedd"
        }

    filter "configurations:Release"
        links
        {
            "shaderc",
            "shaderc_util",
            "shaderc_shared",
            "shaderc_combined"
        }

    filter "action:vs*"
        flags { "MultiProcessorCompile" }