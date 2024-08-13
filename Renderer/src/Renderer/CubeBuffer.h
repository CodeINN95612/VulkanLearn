#pragma once

#include "Renderer/Vulkan/Defines.h"

namespace vl::core
{
	struct CubeRenderData
	{
		glm::mat4 Model;
		glm::vec4 Color;
	};

	class CubeBuffer
	{
	public:
		CubeBuffer(size_t reserve);
		virtual ~CubeBuffer();

		void InsertCubes(const std::vector<CubeRenderData>& insertedCubes);
		void InsertCube(const CubeRenderData& insertedCubes);

		size_t Count() const { return count; }

		CubeRenderData* Data() { return _cubes; }

	private:
		size_t maximumCubes;
		CubeRenderData* _cubes;
		void* _end;
		size_t count;
	};
}