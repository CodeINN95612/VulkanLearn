#pragma once

#include "Renderer/Vertex.h"

namespace vl::core::mesh
{
	struct Triangle
	{
		std::array<Vertex, 3>  vertices = {{
			{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
		}};
	};
}