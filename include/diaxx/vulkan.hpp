#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace diaxx
{
	struct GLFWContext
	{
		GLFWContext();
		~GLFWContext() { glfwTerminate(); }
	};

	class Vulkan
	{
	public:
		Vulkan() = default;

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

		[[nodiscard]] static std::vector<const char*> get_required_extensions();
		[[nodiscard]] static std::vector<const char*> get_required_layers();

		[[nodiscard]] static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
			vk::DebugUtilsMessageTypeFlagsEXT type,
			const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

		void create_surface(); // 2.2
		void pick_physical_device(); // 2.3
		void create_logical_device(); // 2.4
		void create_swap_chain(); // 2.5

		vk::SurfaceFormatKHR choose_swap_surface_format(
			const std::vector<vk::SurfaceFormatKHR>& available_formats);

		vk::PresentModeKHR choose_swap_present_mode(
			const std::vector<vk::PresentModeKHR>& available_present_modes);

		vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities);

		void create_image_views(); // 2.6
		void create_graphics_pipeline(); // 2.7

		void main_loop(); // 3.

		GLFWContext m_glfw {};
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window {
			nullptr, &glfwDestroyWindow };

		vk::raii::Context m_context {};
		vk::raii::Instance m_instance { nullptr };
		vk::raii::DebugUtilsMessengerEXT m_debug_messenger { nullptr };
		vk::raii::PhysicalDevice m_physical_device { nullptr };
		vk::raii::Device m_device { nullptr };
		std::uint32_t m_graphics_queue_family_index {};
		vk::PhysicalDeviceFeatures m_device_features {};
		vk::raii::Queue m_graphics_queue { nullptr };
		vk::raii::SurfaceKHR m_surface { nullptr };
		std::uint32_t m_present_queue_family_index {};
		vk::raii::Queue m_present_queue { nullptr };
		vk::raii::SwapchainKHR m_swap_chain { nullptr };
		std::vector<vk::Image> m_swap_chain_images {};
		vk::Format m_swap_chain_image_format {};
		vk::Extent2D m_swap_chain_extent {};
		std::vector<vk::raii::ImageView> m_swap_chain_image_views {};
	};
}