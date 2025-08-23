#include "diaxx/constants.hpp"
#include "diaxx/vulkan.hpp"

namespace diaxx
{
	Vulkan::~Vulkan()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	void Vulkan::run()
	{
		initialize_window();
		initialize_vulkan();
		main_loop();
	}

	void Vulkan::initialize_window()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Disable OpenGL
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(constants::g_width, constants::g_height, "Diaxx",
			nullptr, nullptr);
	}

	void Vulkan::initialize_vulkan()
	{
	}

	void Vulkan::main_loop()
	{
		while (!glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}
}