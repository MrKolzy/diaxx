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

		// Disable copy construction and assignment
		Vulkan(const Vulkan&) = delete;
		Vulkan& operator=(const Vulkan&) = delete;

		// Disable move construction and move assignment
		Vulkan(Vulkan&&) = delete;
		Vulkan& operator=(Vulkan&&) = delete;

		void run();

	private:
		void initialize_window(); // 1.

		void initialize_vulkan(); // 2.
		void create_instance(); // 2.1

		void print_extensions_and_layers(std::span<const char* const> required_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const char* const> required_layers,
			std::span<const vk::LayerProperties> vulkan_layers) const;

		void check_required_extensions_and_layers(std::span<const char* const> required_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const char* const> required_layers,
			std::span<const vk::LayerProperties> vulkan_layers) const;

		static std::vector<const char*> get_required_extensions();
		static std::vector<const char*> get_required_layers();

		static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
			vk::DebugUtilsMessageTypeFlagsEXT type,
			const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

		void setup_debug_messenger(); // 2.2

		void main_loop(); // 3.

		GLFWwindow* m_window {};
		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
		vk::raii::DebugUtilsMessengerEXT m_debug_messenger { nullptr };
	};
}