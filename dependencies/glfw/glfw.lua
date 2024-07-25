project "GLFW"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "glfw/include/GLFW/**.h",
        "glfw/src/**.h",
        "glfw/src/**.c",
        "glfw/src/**.m"
    }

    includedirs
    {
        "glfw/include"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        defines
        {
            "_GLFW_WIN32"
        }