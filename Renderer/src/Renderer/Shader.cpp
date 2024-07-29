#include "Shader.h"

#include <shaderc/shaderc.hpp>

#include "Renderer/Common/File.h"

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

	return buffer;
}

namespace vl::core
{
	Shader::Shader(const std::string& name, const std::string& path, const std::vector<uint32_t>& code, ShaderType type) :
		_name(name), _path(path), _code(code), _type(type)
	{
	}

	std::shared_ptr<Shader> Shader::Create(const std::string& name, const std::string& path, ShaderType type)
	{
		spdlog::info("Compilando shader {0}", name);

		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetSourceLanguage(shaderc_source_language_glsl);
		options.SetTargetSpirv(shaderc_spirv_version_1_3);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc_shader_kind kind;
		switch (type)
		{
		case vl::core::ShaderType::Vertex:
			kind = shaderc_glsl_vertex_shader;
			break;
		case vl::core::ShaderType::Fragment:
			kind = shaderc_glsl_fragment_shader;
			break;
		case vl::core::ShaderType::Compute:
			kind = shaderc_glsl_compute_shader;
			break;

		default:
			spdlog::error("Tipo de shader no soportado {0}", uint8_t(type));
			throw std::exception("Tipo de shader no soportado");
		}

		auto glsl = common::readFile(path.c_str());
		auto preprocessed = preprocess(glsl.data(), kind, path.c_str(), options);
		auto code = compile(preprocessed.data(), kind, path.c_str(), options);

		return std::make_shared<Shader>(name, path, code, type);
	}

	VkShaderModule Shader::CreateShaderModule(VkDevice device)
	{
		size_t size = _code.size() * sizeof(uint32_t);
		return vulkan::createShaderModule(device, _code, size);
	}


}
