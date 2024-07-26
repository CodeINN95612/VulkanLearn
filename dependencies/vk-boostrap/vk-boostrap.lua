project "vk-boostrap"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    files
    {
        "vk-boostrap/src/*.h",
        "vk-boostrap/src/*.cpp",
    }

    includedirs
    {
        "vk-boostrap/src/",
        "%{vulkanSDK}/Include"
    }

    libdirs
    {
        "%{vulkanSDK}/Lib"
    }

    links
    {
        "vulkan-1"
    }