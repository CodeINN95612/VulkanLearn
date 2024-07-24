function setCommonLibSettings()
    language "C++"
    cppdialect "C++17"
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
        "%{wks.location}/dependencies/spdlog/include/**.h",
        "%{wks.location}/dependencies/spdlog/src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/dependencies/spdlog/include"
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
        "%{wks.location}/dependencies/glm/glm/**.hpp",
        "%{wks.location}/dependencies/glm/glm/**.inl"
    }