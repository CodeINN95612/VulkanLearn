# VulkanLearn

Learning vulkan

## How to build?

1. Install Vulkan version [1.3.318.0](sdk.lunarg.com/sdk/download/1.3.283.0/windows/vulkan_sdk.exe) with debug libraries if planning to run on debug mode.
2. Clone repo

```bash
git clone https://github.com/CodeINN95612/VulkanLearn.git --recursive
```

> `--recursive` flag installs all the submodules needed, it won't compile correctly without it

3. Excecute premake. If Visual Studio 2022 is installed you can excecute the batch file `gen_vs2022.bat`.

## Submodules

- [Dear Imgui](https://github.com/ocornut/imgui.git)
- [SpdLog](https://github.com/gabime/spdlog.git)
- [GLM](https://github.com/g-truc/glm.git)
- [FastGLFT](https://github.com/spnda/fastgltf)
- [SimdJson](https://github.com/simdjson/simdjson)
- [GLFW](https://github.com/glfw/glfw)
