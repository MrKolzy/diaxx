#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

Vulkan::Vulkan()
{
	start();
}

void Vulkan::start()
{
	initialize_window();
	initialize_vulkan();
	main_loop();
	cleanup();
}

void Vulkan::initialize_window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify that's not an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Make the window non-resizable

	m_window = glfwCreateWindow(constants::g_width, constants::g_height, "diaxx", nullptr
		, nullptr);
}

void Vulkan::initialize_vulkan()
{
}

void Vulkan::main_loop()
{
	while (!glfwWindowShouldClose(m_window))
		glfwPollEvents();
}

void Vulkan::cleanup()
{
	glfwDestroyWindow(m_window);

	glfwTerminate();
}