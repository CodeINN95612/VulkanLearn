#pragma once

#include "Renderer/Vertex.h"

namespace vl::core::mesh
{
	struct Square
	{
		std::vector<Vertex> vertices = { {
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
			{{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
		}};

		std::vector<uint16_t> indices = {
			0, 1, 2, 2, 3, 0
		};
	};
}