project "fastgltf"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "fastgltf/include/fastgltf/*.hpp",
        "fastgltf/src/*.cpp",
        "fastgltf/src/*.ixx",
    }

    includedirs
    {
        "fastgltf/include",
        "%{wks.location}/dependencies/simdjson/simdjson/singleheader",
    }

    links
    {
        "simdjson"
    }