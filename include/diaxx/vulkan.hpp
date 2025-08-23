#pragma once

#include <GLFW/glfw3.h>

namespace diaxx
{
	class Vulkan
	{
	public:
		Vulkan() = default;
		~Vulkan();

		void run();

	private:
		void initialize_window();
		void initialize_vulkan();
		void main_loop();

		GLFWwindow* m_window {};
	};
}