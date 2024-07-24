#include "Test.h"

#include <imgui/imgui.h>

int main()
{
	add(1, 2);
	spdlog::info("Hello, World!");

	auto tmp = ImGui::GetVersion();

	return 0;
}