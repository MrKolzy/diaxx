#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

#include <cstdint>
#include <cstring>
#include <print>
#include <stdexcept>
#include <vector>

Vulkan::~Vulkan()
{
	cleanup();
}

void Vulkan::start()
{
	initialize_window();
	initialize_vulkan();
	main_loop();
}

void Vulkan::initialize_window()
{
	if (!glfwInit())
		throw std::runtime_error("[Error]: Failed to initialize GLFW.");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Specify that's not an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // Make the window non-resizable

	m_window = glfwCreateWindow(constants::g_width, constants::g_height, "Diaxx", nullptr, nullptr);
	if (!m_window)
		throw std::runtime_error("[Error]: Failed to create GLFW window.");
}

void Vulkan::initialize_vulkan()
{
	create_instance(true);
}

void Vulkan::create_instance(bool show_extensions)
{
	// Useful information to the driver in order to optimize our specific application
	const VkApplicationInfo application_information { 
		.sType              { VK_STRUCTURE_TYPE_APPLICATION_INFO },
		.pApplicationName   { "Diaxx"                            },
		.applicationVersion { VK_MAKE_VERSION(1, 0, 0)           },
		.pEngineName        { "Diaxx Engine"                     },
		.engineVersion      { VK_MAKE_VERSION(1, 0, 0)           },
		.apiVersion         { VK_API_VERSION_1_0                 }
	};

	// Tells the Vulkan driver which global extensions and validation layers we want to use
	std::uint32_t glfw_extension_count {};
	const char**  glfw_extension_names { glfwGetRequiredInstanceExtensions(&glfw_extension_count) };

	const VkInstanceCreateInfo create_info {
		.sType                   { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
		.pApplicationInfo        { &application_information               },
		.enabledLayerCount       { 0                                      },
		.enabledExtensionCount   { glfw_extension_count                   },
		.ppEnabledExtensionNames { glfw_extension_names                   }
	};

	// Retrieve a list of supported extensions before creating an instance
	if (show_extensions)
	{	
		// Request the number of extensions
		std::uint32_t extension_count {};
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

		// Request the extension details
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

		std::println("[Debug]: List of available extensions:");
		for (const auto& extension : extensions)
			std::println("\t- {}", extension.extensionName);

		std::println("\n[Debug]: List of extensions needed by GLFW:");
		for (std::uint32_t i { 0 }; i < glfw_extension_count; ++i)
			std::println("\t- {}", glfw_extension_names[i]);

		std::println("\n[Debug]: List of GLFW extensions found:");
		for (std::uint32_t i { 0 }; i < glfw_extension_count; ++i)
		{
			const char* glfw_extension_name { glfw_extension_names[i] };

			for (const auto& extension : extensions)
			{
				if (!std::strcmp(glfw_extension_name, extension.extensionName))
				{
					std::println("\t- {}", extension.extensionName);
					break;
				}
			}
		}
	}

	const VkResult result { vkCreateInstance(&create_info, nullptr, &m_instance) };
	if (result != VK_SUCCESS)
		throw std::runtime_error("[Error]: The instance could not be created.");
}

void Vulkan::main_loop()
{
	while (!glfwWindowShouldClose(m_window))
		glfwPollEvents();
}

void Vulkan::cleanup()
{
	if (m_instance)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = nullptr;
	}
	
	if (m_window)
	{
		glfwDestroyWindow(m_window);
		m_window = nullptr;
	}
	
	glfwTerminate();
}