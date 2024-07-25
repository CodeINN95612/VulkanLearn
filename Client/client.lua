project "Client"
    kind "ConsoleApp"
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
        "%{wks.location}/Renderer/src",
        "%{wks.location}/include",
        "%{wks.location}/dependencies/spdlog/spdlog/include",
        "%{wks.location}/dependencies/glm/glm",
        "%{wks.location}/dependencies/imgui",
        "%{wks.location}/dependencies/glfw/glfw/include",
    }

    links
    {
        "Renderer",
        "imgui",
        "glfw"
    }