project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "spdlog/include/**.h",
        "spdlog/src/**.cpp"
    }

    includedirs
    {
        "spdlog/include"
    }

    defines
    {
        "SPDLOG_COMPILED_LIB"
    }