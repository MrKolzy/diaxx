#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <span>

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
		void print_extensions_and_layers(std::span<const char* const> glfw_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const vk::LayerProperties> vulkan_layers);
		void check_required_extensions_and_layers(std::span<const char* const> glfw_extensions,
			std::span<const vk::ExtensionProperties> vulkan_extensions,
			std::span<const vk::LayerProperties> vulkan_layers);

		void main_loop();

		GLFWwindow* m_window {};
		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
	};
}