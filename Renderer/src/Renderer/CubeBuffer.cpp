#include "CubeBuffer.h"

namespace vl::core
{
	CubeBuffer::CubeBuffer(size_t reserve) : 
		maximumCubes(reserve), 
		count(0)
	{
		_cubes = new CubeRenderData[reserve];
		_end = _cubes;
	}

	CubeBuffer::~CubeBuffer()
	{
		delete[] _cubes;
	}

	void CubeBuffer::InsertCubes(const std::vector<CubeRenderData>& insertedCubes)
	{
		size_t size = insertedCubes.size() * sizeof(CubeRenderData);
		memcpy(_end, insertedCubes.data(), size);
		_end = (void*)((size_t)_end + size);
		count += insertedCubes.size();
	}

	void CubeBuffer::InsertCube(const CubeRenderData& insertedCubes)
	{
		memcpy(_end, &insertedCubes, sizeof(CubeRenderData));
		_end = (void*)((size_t)_end + sizeof(CubeRenderData));
		count++;
	}
}