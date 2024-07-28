#pragma once

#include <vector>
#include <fstream>

#include <spdlog/spdlog.h>

namespace vl::core::common
{
	inline static std::vector<char> readFile(const char* filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			spdlog::error("No se pudo abrir el archivo {0}", filename);
			throw std::exception("No se pudo abrir el archivo");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize + 1);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		buffer[fileSize] = '\0';
		file.close();

		return buffer;
	}
}
