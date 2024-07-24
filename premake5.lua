workspace "VulkanLearn"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Client"

outputdir = "%{cfg.buildcfg}"

-- Workspace-wide settings
filter "system:windows"
    systemversion "latest"
    defines { "VKL_WINDOWS" }

filter "configurations:Debug"
    defines { "DEBUG" }
    runtime "Debug"
    symbols "on"

filter "configurations:Release"
    defines { "NDEBUG" }
    runtime "Release"
    optimize "on"

group "Dependencies"
    -- Include dependencies
    include "dependencies/dependencies.lua"
group ""

-- Include Engine project
include "Renderer/renderer.lua"

-- Include Client project
include "Client/client.lua"