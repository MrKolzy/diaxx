#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

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
		void create_instance();

		void primary_loop();

		void cleanup();

		GLFWwindow* m_window {};
		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
	};
}