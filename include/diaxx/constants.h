#pragma once

#include <array>

namespace Constants
{
	inline constexpr int g_width { 800 };
	inline constexpr int g_height { 600 };
	inline constexpr std::array<const char*, 1> g_validation_layers {
		"VK_LAYER_KHRONOS_validation" };
#ifndef NDEBUG
	inline constexpr bool g_enable_validation_layers { true };
#else
	inline constexpr bool g_enable_validation_layers { false };
#endif
}