#include "Test.h"

int add(int a, int b)
{
	glm::vec3 test(1.0f, 2.0f, 3.0f);
	spdlog::info("Test: {} {}", test.x, a + b);

	return a + b;
}