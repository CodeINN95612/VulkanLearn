include "vulkan_version.lua"

workspace "VulkanLearn"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Client"
    platforms { "Win64" }

outputdir = "%{cfg.buildcfg}"
OutDir = "%{wks.location}/bin/" .. outputdir .. "/%{prj.name}"
ObjDir = "%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}"

filter "configurations:Debug"
    defines { "DEBUG" }
    runtime "Debug"
    symbols "on"

filter "configurations:Release"
    defines { "NDEBUG" }
    runtime "Release"
    optimize "on"

group "Dependencies"
    include "dependencies/dependencies.lua"
group ""

group "Core"
    include "Renderer/renderer.lua"
group ""

include "Client/client.lua"