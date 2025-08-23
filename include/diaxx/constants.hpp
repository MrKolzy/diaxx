#pragma once

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
}