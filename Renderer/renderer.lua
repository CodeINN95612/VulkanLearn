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
        "%{wks.location}/dependencies/imgui"
    }

    libdirs
    {
        "%{wks.location}/lib"
    }

    links
    {
        "vulkan-1",
        "imgui"
    }