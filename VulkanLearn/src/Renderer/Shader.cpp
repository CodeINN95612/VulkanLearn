#include "Shader.hpp"

#include <shaderc/shaderc.hpp>

#include "../Vulkan/Pipeline.hpp"

struct CompilationInfo 
{
    const char* fileName;
    shaderc_shader_kind kind;
    std::vector<char> source;
    shaderc::CompileOptions options;
};

static std::vector<char> preprocess(const char* code, shaderc_shader_kind kind, const char* path, const shaderc::CompileOptions& options)
{
	shaderc::Compiler compiler;
	shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(code, kind, path, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		spdlog::error("Error al compilar el shader '{0}' con error: '{1}'", path, result.GetErrorMessage().c_str());
		throw std::exception(result.GetErrorMessage().c_str());
	}

	const char* src = result.cbegin();
	size_t newSize = result.cend() - src;
	std::vector<char> buffer(newSize + 1);
	memcpy(buffer.data(), src, newSize);
	buffer[newSize] = '\0';

	return buffer;
}

static std::vector<char> compileToAssembly(const char* code, shaderc_shader_kind kind, const char* path, const shaderc::CompileOptions& options)
{
	shaderc::Compiler compiler;
	shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(code, kind, path, options);

	auto result2 = compiler.CompileGlslToSpv(code, kind, path, options);
	const uint32_t* src2 = result2.cbegin();

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		spdlog::error("Error al compilar el shader '{0}' con error: '{1}'", path, result.GetErrorMessage().c_str());
		throw std::exception(result.GetErrorMessage().c_str());
	}

	const char* src = result.cbegin();
	size_t newSize = result.cend() - src;
	std::vector<char> buffer(newSize + 1);
	memcpy(buffer.data(), src, newSize);
	buffer[newSize] = '\0';

	return buffer;
}

static std::vector<uint32_t> compile(const char* code, shaderc_shader_kind kind, const char* path, const shaderc::CompileOptions& options)
{
	shaderc::Compiler compiler;
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(code, kind, path, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		spdlog::error("Error al compilar el shader '{0}' con error: '{1}'", path, result.GetErrorMessage().c_str());
		throw std::exception(result.GetErrorMessage().c_str());
	}

	const uint32_t* src = result.cbegin();
	size_t newSize = result.cend() - src;
	std::vector<uint32_t> buffer(newSize);
	memcpy(buffer.data(), src, newSize * sizeof(uint32_t));

	return buffer;
}

static std::vector<uint32_t> assemble(const char* compiledCode, size_t size, const shaderc::CompileOptions& options)
{
	shaderc::Compiler compiler;
	shaderc::SpvCompilationResult result = compiler.AssembleToSpv(compiledCode, size, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		spdlog::error("Error al ensamblar el shader con error: '{0}'", result.GetErrorMessage().c_str());
		throw std::exception(result.GetErrorMessage().c_str());
	}

	std::vector<uint32_t> buffer = { result.cbegin(), result.cend() };

	spdlog::debug("---- SPIR-V Binary Code ----");
	spdlog::debug("Magic Number: {0} vs 119734787", buffer[0]);

	return buffer;
}

namespace Renderer
{
	Shader::Shader(
		const std::string& name, 
		const std::string& vertexPath, 
		const std::string& fragmentPath,
		const std::vector<uint32_t>& vertexCode,
		const std::vector<uint32_t>& fragmentCode) :
		_name(name), 
		_vertexPath(vertexPath), 
		_fragmentPath(fragmentPath),
		_vertexCode(vertexCode),
		_fragmentCode(fragmentCode)
	{

	}

    std::shared_ptr<Shader> Shader::Create(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
    {
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetSourceLanguage(shaderc_source_language_glsl);
		options.SetTargetSpirv(shaderc_spirv_version_1_3);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		auto vertexCode = Common::Utils::readFile(vertexPath.c_str());
		auto vertexPreprocessed = preprocess(vertexCode.data(), shaderc_glsl_vertex_shader, vertexPath.c_str(), options);
		auto vertexCompiled = compile(vertexPreprocessed.data(), shaderc_glsl_vertex_shader, vertexPath.c_str(), options);
		//auto vertexAssembled = assemble(vertexCompiled.data(), vertexCompiled.size(), options);

		auto fragmentCode = Common::Utils::readFile(fragmentPath.c_str());
		auto fragmentPreprocessed = preprocess(fragmentCode.data(), shaderc_glsl_fragment_shader, fragmentPath.c_str(), options);
		auto fragmentCompiled = compile(fragmentPreprocessed.data(), shaderc_glsl_fragment_shader, fragmentPath.c_str(), options);
		//auto fragmentAssembled = assemble(fragmentCompiled.data(), fragmentCompiled.size(), options);

		auto shader = std::make_shared<Shader>(name, vertexPath, fragmentPath, vertexCompiled, fragmentCompiled);
			
        return shader;
    }

    VkShaderModule Shader::BuildModule(VkDevice device, ShaderType type) const
    {
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		switch (type)
		{
			case ShaderType::Vertex:
				shaderModule = Vulkan::Pipeline::createShaderModule(device, _vertexCode, sizeof(uint32_t) * _vertexCode.size());
				break;

			case ShaderType::Fragment:
				shaderModule = Vulkan::Pipeline::createShaderModule(device, _fragmentCode, sizeof(uint32_t) * _fragmentCode.size());
				break;
		}

		return shaderModule;
    }


	ComputeShader::ComputeShader(const std::string& name, const std::string& path, const std::vector<uint32_t>& code) :
		_name(name),
		_path(path),
		_code(code)
	{
	}

	std::shared_ptr<ComputeShader> ComputeShader::Create(const std::string& name, const std::string& path)
	{
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetSourceLanguage(shaderc_source_language_glsl);
		options.SetTargetSpirv(shaderc_spirv_version_1_3);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		auto code = Common::Utils::readFile(path.c_str());
		auto preprocessed = preprocess(code.data(), shaderc_glsl_compute_shader, path.c_str(), options);
		auto compiled = compile(preprocessed.data(), shaderc_glsl_compute_shader, path.c_str(), options);
		//auto assembled = assemble(compiled.data(), path.size(), options);

		auto shader = std::make_shared<ComputeShader>(name, path, compiled);

		return shader;
	}

	VkShaderModule ComputeShader::BuildModule(VkDevice device) const
	{
		return Vulkan::Pipeline::createShaderModule(device, _code, sizeof(uint32_t) * _code.size());
	}

}
