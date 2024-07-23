#pragma once
#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

namespace Renderer
{
	enum class ShaderType : uint8_t
	{
		Vertex = 1,
		Fragment = 2
	};

	class Shader
	{
	public:
		Shader(
			const std::string& name,
			const std::string& vertexPath,
			const std::string& fragmentPath,
			const std::vector<uint32_t>& vertexCode,
			const std::vector<uint32_t>& fragmentCode);
		virtual ~Shader() = default;

		static std::shared_ptr<Shader> Create(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);

		VkShaderModule BuildModule(VkDevice device, ShaderType type) const;

	private:
		std::string _name;
		std::string _vertexPath;
		std::string _fragmentPath;
		std::vector<uint32_t> _vertexCode;
		std::vector<uint32_t> _fragmentCode;
	};

	class ComputeShader
	{
	public:
		ComputeShader(
			const std::string& name,
			const std::string& path,
			const std::vector<uint32_t>& code);
		virtual ~ComputeShader() = default;

		static std::shared_ptr<ComputeShader> Create(const std::string& name, const std::string& path);

		VkShaderModule BuildModule(VkDevice device) const;


	private:
		std::string _name;
		std::string _path;
		std::vector<uint32_t> _code;
	};
}