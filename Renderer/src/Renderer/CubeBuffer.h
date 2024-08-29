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

		void UpdateCube(size_t index, const CubeRenderData& cube);

		CubeRenderData* Data() { return _cubes; }
		const CubeRenderData* Data() const { return _cubes; }

		const std::vector<size_t>& UpdatedIndices() const { return updatedIndices; }
		inline void ClearUpdatedIndices() { updatedIndices.clear(); }

	private:

		size_t maximumCubes;
		CubeRenderData* _cubes;
		std::vector<size_t> updatedIndices;
	};
}