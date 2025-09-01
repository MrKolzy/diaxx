#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>

#include <array>

namespace constants
{
	inline constexpr int g_width { 800 };
	inline constexpr int g_height { 600 };

	inline constexpr std::array<const char*, 1> g_required_layers {
		"VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
	inline constexpr bool g_enable_validation_layers { false };
#else
	inline constexpr bool g_enable_validation_layers { true };
#endif

	inline constexpr std::array<const char*, 5> g_device_extensions {
		"VK_KHR_swapchain",	"VK_KHR_spirv_1_4", "VK_KHR_synchronization2",
		"VK_KHR_create_renderpass2", "VK_KHR_shader_draw_parameters" };

	inline constexpr vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

	inline constexpr vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
}