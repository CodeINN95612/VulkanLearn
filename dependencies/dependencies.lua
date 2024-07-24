function setCommonLibSettings()
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
end

project "spdlog"
    kind "StaticLib"
    setCommonLibSettings()

    files
    {
        "spdlog/spdlog/include/**.h",
        "spdlog/spdlog/src/**.cpp"
    }

    includedirs
    {
        "spdlog/spdlog/include"
    }

    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

project "glm"
    kind "StaticLib"
    setCommonLibSettings()

    files
    {
        "glm/glm/glm/**.hpp",
        "glm/glm/glm/**.inl"
    }