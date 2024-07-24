project "glm"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "glm/glm/**.hpp",
        "glm/glm/**.inl"
    }