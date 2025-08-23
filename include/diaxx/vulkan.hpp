#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <span>
#include <vector>

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
		void create_instance();
		void print_extensions_and_layers(std::span<const char* const> required_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const char* const> required_layers,
			std::span<const vk::LayerProperties> vulkan_layers);
		void check_required_extensions_and_layers(std::span<const char* const> required_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const char* const> required_layers,
			std::span<const vk::LayerProperties> vulkan_layers);
		std::vector<const char*> get_required_extensions();
		std::vector<const char*> get_required_layers();
		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
			vk::DebugUtilsMessageTypeFlagsEXT type,
			const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
		void setup_debug_messenger();

		void main_loop();

		GLFWwindow* m_window {};
		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
		vk::raii::DebugUtilsMessengerEXT m_debug_messenger { nullptr };
	};
}