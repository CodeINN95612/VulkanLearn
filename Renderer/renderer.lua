project "Renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir (OutDir)
    objdir (ObjDir)

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "src",
        "%{wks.location}/dependencies/spdlog/spdlog/include",
        "%{wks.location}/dependencies/glm/glm",
        "%{wks.location}/dependencies/imgui",
        "%{wks.location}/dependencies/fastgltf/fastgltf/include",
        "%{wks.location}/dependencies/glfw/glfw/include",
        "%{VULKAN_SDK}/Include"
    }

    vulkanSDK = os.getenv("VULKAN_SDK")

    libdirs
    {
        "%{vulkanSDK}/Lib"
    }

    links
    {
        "vulkan-1",
        "imgui",
        "fastgltf",
        "glfw"
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