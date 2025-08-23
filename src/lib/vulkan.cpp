#include "diaxx/constants.hpp"
#include "diaxx/vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>

namespace diaxx
{
	Vulkan::~Vulkan()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	void Vulkan::run()
	{
		initialize_window();
		initialize_vulkan();
		main_loop();
	}

	void Vulkan::initialize_window()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Disable OpenGL
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(constants::g_width, constants::g_height, "Diaxx",
			nullptr, nullptr);
	}

	void Vulkan::initialize_vulkan()
	{
		// Enables communication between the application and Vulkan
		create_instance();
	}

	void Vulkan::create_instance()
	{
		constexpr vk::ApplicationInfo application_info {
			.pApplicationName = "Diaxx",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Diaxx Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14
		};

		std::uint32_t glfw_extension_count {};
		const auto glfw_extensions { glfwGetRequiredInstanceExtensions(&glfw_extension_count) };
		const auto vulkan_extensions { m_context.enumerateInstanceExtensionProperties() };
		const auto vulkan_layers { m_context.enumerateInstanceLayerProperties() };

		print_extensions_and_layers(std::span { glfw_extensions, glfw_extension_count },
			std::span { vulkan_extensions }, std::span { vulkan_layers });
		check_required_extensions_and_layers(std::span { glfw_extensions, glfw_extension_count },
			std::span { vulkan_extensions }, std::span { vulkan_layers });

		// Structure with the information Vulkan needs
		const vk::InstanceCreateInfo create_info {
			.pApplicationInfo = &application_info,
			.enabledExtensionCount = glfw_extension_count,
			.ppEnabledExtensionNames = glfw_extensions
		};

		m_instance = vk::raii::Instance(m_context, create_info);
	}

	void Vulkan::print_extensions_and_layers(std::span<const char* const> glfw_extensions,
		std::span<const vk::ExtensionProperties> vulkan_extensions,
		std::span<const vk::LayerProperties> vulkan_layers)
	{
		std::println("[Debug]: Available Vulkan extensions:");
		for (const auto& extension : vulkan_extensions)
			std::println("\t - {}", std::string_view { extension.extensionName });

		std::println("\n[Debug]: Required GLFW extensions:");
		for (const auto& extension : glfw_extensions)
			std::println("\t - {}", std::string_view { extension });

		std::println("\n[Debug]: Available Vulkan layers:");
		for (const auto& layer : vulkan_layers)
			std::println("\t - {}", std::string_view { layer.layerName });
	}

	void Vulkan::check_required_extensions_and_layers(std::span<const char* const> glfw_extensions,
		std::span<const vk::ExtensionProperties> vulkan_extensions,
		std::span<const vk::LayerProperties>)
	{
		for (const auto& extension : glfw_extensions)
		{
			if (std::ranges::none_of(vulkan_extensions,
				[extension](const auto& vulkan_extension)
				{
					return std::string_view { vulkan_extension.extensionName } == extension;
				}))
			{
				throw std::runtime_error(std::format(
					"[Error]: The {} extension required by GLFW is not supported.",
					extension));
			}
		}
	}

	void Vulkan::main_loop()
	{
		while (!glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}
}