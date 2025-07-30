#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <print>
#include <stdexcept>
#include <vector>

namespace
{
	// Proxy Function
	VkResult create_debug_utils_messenger_ext(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* create_info,
		const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger)
	{
		const auto function { reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")) };

		if (function != nullptr)
			return function(instance, create_info, allocator, debug_messenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	// Proxy Function
	void destroy_debug_utils_messenger_ext(VkInstance instance,
		VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator)
	{
		const auto function { reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")) };

		if (function != nullptr)
			function(instance, debug_messenger, allocator);
	}
}

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
	setup_debug_messenger();
}

void Vulkan::create_instance(bool show_extensions)
{
	if (constants::g_enable_validation_layers && !check_validation_layer_support())
		throw std::runtime_error("[Error]: Validation layers requested but not available.");

	// Useful information to the driver in order to optimize our specific application
	const VkApplicationInfo application_information { 
		.sType              { VK_STRUCTURE_TYPE_APPLICATION_INFO },
		.pApplicationName   { constants::g_application_name      },
		.applicationVersion { VK_MAKE_VERSION(1, 0, 0)           },
		.pEngineName        { constants::g_engine_name           },
		.engineVersion      { VK_MAKE_VERSION(1, 0, 0)           },
		.apiVersion         { VK_API_VERSION_1_0                 }
	};

	// Tells the Vulkan driver which global extensions and validation layers we want to use
	auto required_extensions = get_required_extensions();
	const std::uint32_t glfw_extension_count { static_cast<std::uint32_t>(required_extensions.size()) };
	const char**  glfw_extension_names { required_extensions.data() };

	VkInstanceCreateInfo create_info {
		.sType                   { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
		.pNext                   { nullptr                                },
		.pApplicationInfo        { &application_information               },
		.enabledLayerCount       { 0                                      },
		.enabledExtensionCount   { glfw_extension_count                   },
		.ppEnabledExtensionNames { glfw_extension_names                   }
	};

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};

	if (constants::g_enable_validation_layers)
	{
		create_info.enabledLayerCount = { static_cast<std::uint32_t>(
			constants::g_validation_layers.size()) };
		create_info.ppEnabledLayerNames = { constants::g_validation_layers.data() };

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = { &debug_create_info };
	}

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

		std::println("");
	}

	const VkResult result { vkCreateInstance(&create_info, nullptr, &m_instance) };
	if (result != VK_SUCCESS)
		throw std::runtime_error("[Error]: The instance could not be created.");
}

void Vulkan::setup_debug_messenger()
{
	if (!constants::g_enable_validation_layers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT create_info {};
	populate_debug_messenger_create_info(create_info);

	if (create_debug_utils_messenger_ext(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to set up debug messenger.");
}

void Vulkan::pick_physical_device()
{
}

bool Vulkan::check_validation_layer_support()
{
	std::uint32_t layer_count {};
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : constants::g_validation_layers)
	{
		bool layer_found { false };

		for (const auto& layer_properties : available_layers)
		{
			if (!strcmp(layer_name, layer_properties.layerName))
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

std::vector<const char*> Vulkan::get_required_extensions()
{
	std::uint32_t glfw_extension_count {};
	const char** glfw_extensions { glfwGetRequiredInstanceExtensions(&glfw_extension_count) };

	if (!glfw_extensions)
		throw std::runtime_error("[Error]: Failed to get required GLFW extensions.");

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (constants::g_enable_validation_layers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void Vulkan::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {
		.sType           { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT },
		.messageSeverity { VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT                          },
		.messageType     { VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT                        },
		.pfnUserCallback { debug_callback                                          },
		.pUserData       { nullptr                                                 }
	};
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void*)
{
	std::cerr << "[Validation Layer]: " << callback_data->pMessage << '\n';

	return VK_FALSE;
}

void Vulkan::main_loop()
{
	while (!glfwWindowShouldClose(m_window))
		glfwPollEvents();
}

void Vulkan::cleanup()
{
	if (constants::g_enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(m_instance, m_debug_messenger, nullptr);
		m_debug_messenger = nullptr;
	}
		
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