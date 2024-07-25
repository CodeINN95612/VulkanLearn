project "simdjson"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "simdjson/singleheader/simdjson.h",
        "simdjson/singleheader/simdjson.cpp",
    }

    includedirs
    {
        "simdjson/singleheader",
    }