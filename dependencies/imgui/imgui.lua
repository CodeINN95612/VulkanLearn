project "imgui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (OutDir)
    objdir (ObjDir)
        
    includedirs
    {
        "imgui",
        "%{wks.location}/dependencies/glfw/glfw/include",
        "%{vulkanSDK}/Include"
    }

    files
    {
        "imgui/imconfig.h",
        "imgui/imgui.h",
        "imgui/imgui.cpp",
        "imgui/imgui_draw.cpp",
        "imgui/imgui_internal.h",
        "imgui/imgui_tables.cpp",
        "imgui/imgui_widgets.cpp",
        "imgui/imstb_rectpack.h",
        "imgui/imstb_textedit.h",
        "imgui/imstb_truetype.h",
        "imgui/imgui_demo.cpp",
        "imgui/backends/imgui_impl_glfw.h",
        "imgui/backends/imgui_impl_glfw.cpp",
        "imgui/backends/imgui_impl_vulkan.h",
        "imgui/backends/imgui_impl_vulkan.cpp",
    }

    libdirs
    {
        "%{vulkanSDK}/Lib"
    }

    links
    {
        "vulkan-1",
        "glfw",
    }