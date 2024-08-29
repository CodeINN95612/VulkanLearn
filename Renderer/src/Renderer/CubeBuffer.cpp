#include "CubeBuffer.h"

namespace vl::core
{
	CubeBuffer::CubeBuffer(size_t reserve) : 
		maximumCubes(reserve)
	{
		_cubes = new CubeRenderData[reserve];
		memset(_cubes, 0, reserve * sizeof(CubeRenderData));
	}

	CubeBuffer::~CubeBuffer()
	{
		delete[] _cubes;
	}

	void CubeBuffer::UpdateCube(size_t index, const CubeRenderData& cube)
	{
		memcpy(&_cubes[index], &cube, sizeof(CubeRenderData));
		updatedIndices.push_back(index);
	}
}