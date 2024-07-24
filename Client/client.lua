project "Client"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

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
        "%{wks.location}/dependencies/glm/glm"
    }

    links
    {
        "Renderer",
    }