#pragma once

#include <GLFW/glfw3.h>

namespace Diaxx
{
	class Vulkan
	{
	public:
		Vulkan() = default;
		~Vulkan();

		void start();

	private:
		void initialize_window();
		void initialize_vulkan();
		void primary_loop();
		void cleanup();

		GLFWwindow* m_window {};
	};
}