#pragma once

#include <vulkan/vulkan_core.h>

#include <array>

namespace constants
{
#ifdef NDEBUG
	inline constexpr bool g_enable_validation_layers { false };
#else
	inline constexpr bool g_enable_validation_layers { true };
#endif

	inline constexpr int g_width  { 800 };
	inline constexpr int g_height { 600 };
	inline constexpr std::array<const char*, 1> g_validation_layers {
		"VK_LAYER_KHRONOS_validation" };
	inline constexpr char g_application_name[] { "Diaxx"        };
	inline constexpr char g_engine_name[]      { "Diaxx Engine" };
	inline constexpr std::array<const char*, 1> g_device_extensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	inline constexpr int g_max_frames_in_flight { 2 };
}