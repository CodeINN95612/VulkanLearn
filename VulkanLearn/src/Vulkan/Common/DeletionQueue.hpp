#pragma once
#include <deque>
#include <functional>

namespace Vulkan::Common
{
	class DeletionQueue
	{
	public:
		std::deque<std::function<void()>> deletors;

		inline void Push(std::function<void()>&& function) {
			deletors.push_back(function);
		}

		void Flush() {
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)();
			}

			deletors.clear();
		}
	};

}