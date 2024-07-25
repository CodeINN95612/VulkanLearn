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
        "%{wks.location}/include",
        "%{wks.location}/dependencies/spdlog/spdlog/include",
        "%{wks.location}/dependencies/glm/glm",
        "%{wks.location}/dependencies/imgui",
        "%{wks.location}/dependencies/fastgltf/fastgltf/include",
        "%{wks.location}/dependencies/glfw/glfw/include"
    }

    libdirs
    {
        "%{wks.location}/lib"
    }

    links
    {
        "vulkan-1",
        "imgui",
        "fastgltf",
        "glfw"
    }