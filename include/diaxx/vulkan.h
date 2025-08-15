#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>

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
		std::vector<const char*> get_required_extensions();
		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
			vk::DebugUtilsMessageTypeFlagsEXT message_type,
			const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
		void setup_debug_messenger();

		void primary_loop();

		void cleanup();

		GLFWwindow* m_window {};
		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
		vk::raii::DebugUtilsMessengerEXT m_debug_messenger { nullptr };
	};
}