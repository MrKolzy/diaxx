#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

#include <cstdint>

namespace Diaxx
{
	Vulkan::~Vulkan()
	{
		cleanup();
	}

	void Vulkan::start()
	{
		initialize_window();
		initialize_vulkan();
		primary_loop();
	}

	void Vulkan::initialize_window()
	{
		glfwInit();

		// Specify that's not an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window = glfwCreateWindow(Constants::g_width, Constants::g_height,
			"Diaxx", nullptr, nullptr);
	}

	void Vulkan::initialize_vulkan()
	{
		// The instance is the connection between the application and the Vulkan library
		create_instance();
	}

	void Vulkan::create_instance()
	{
		// Optional structure with information about the application
		constexpr vk::ApplicationInfo application_info("Diaxx", VK_MAKE_VERSION(1, 0, 0),
			"Diaxx Engine", VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion14);

		// GLFW asks Vulkan which instance extensions it needs
		std::uint32_t glfw_extension_count {};
		const auto glfw_extensions { glfwGetRequiredInstanceExtensions(&glfw_extension_count) };

		// Check that GLFW instance extensions are compatible with Vulkan instance extensions
		const auto vulkan_extensions { m_context.enumerateInstanceExtensionProperties() };
		for (std::uint32_t i {}; i < glfw_extension_count; ++i)
		{
			if (std::ranges::none_of(vulkan_extensions,
				[glfw_extension = glfw_extensions[i]](auto const& vulkan_extension)
				{ return !std::strcmp(glfw_extension, vulkan_extension.extensionName); }))
			{
				throw std::runtime_error(std::format(
					"[Error] The GLFW instance extension {} is not supported.", glfw_extensions[i]));
			}
		}

		// Tell Vulkan which global extensions and validation layers we will use
		const vk::InstanceCreateInfo create_info({}, &application_info, 0, nullptr,
			glfw_extension_count, glfw_extensions);

		// RAII call to create the Vulkan instance
		m_instance = vk::raii::Instance(m_context, create_info);
	}

	void Vulkan::primary_loop()
	{
		while (!glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}

	void Vulkan::cleanup()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}
}