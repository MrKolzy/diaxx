#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

namespace Diaxx
{
	Vulkan::~Vulkan()
	{
		cleanup();
	}

	void Vulkan::start()
	{
		initialize_window();
		initialize_vulkan();
		primary_loop();
	}

	void Vulkan::initialize_window()
	{
		glfwInit();

		// Specify that's not an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(Constants::g_width, Constants::g_height,
			"Diaxx", nullptr, nullptr);
	}

	void Vulkan::initialize_vulkan()
	{
	}

	void Vulkan::primary_loop()
	{
		while (!glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}

	void Vulkan::cleanup()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}
}