#pragma once

#include "Renderer/Vulkan/Vulkan.h"

namespace vl::core
{

	enum class ShaderType : uint8_t
	{
		Vertex = 1,
		Fragment = 2,
		Compute = 3,
	};

	class Shader
	{
	public:
		Shader(const std::string& name,
			const std::string& path,
			const std::vector<uint32_t>& code,
			ShaderType type);

		virtual ~Shader() = default;

		static std::shared_ptr<Shader> Create(const std::string& name,
			const std::string& path,
			ShaderType type);

		VkShaderModule CreateShaderModule(VkDevice device);

	private:
		std::string _name;
		std::string _path;
		std::vector<uint32_t> _code;
		ShaderType _type;
	};

}
