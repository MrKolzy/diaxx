#include "diaxx/constants.hpp"
#include "diaxx/vulkan.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <iostream>
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
		// Enables Vulkan debug messages for validation
		setup_debug_messenger();
		// Select a suitable GPU (physical device) that supports the required Vulkan features
		pick_physical_device();
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

		const auto required_extensions { get_required_extensions() };
		const auto vulkan_extensions { m_context.enumerateInstanceExtensionProperties() };

		const auto required_layers { get_required_layers() };
		const auto vulkan_layers { m_context.enumerateInstanceLayerProperties() };

		print_extensions_and_layers(std::span { required_extensions },
			std::span { vulkan_extensions }, std::span { required_layers },
			std::span { vulkan_layers });
		check_required_extensions_and_layers(std::span { required_extensions },
			std::span { vulkan_extensions }, std::span { required_layers },
			std::span { vulkan_layers });

		// Structure with the information Vulkan needs
		const vk::InstanceCreateInfo create_info {
			.pApplicationInfo = &application_info,
			.enabledLayerCount = static_cast<std::uint32_t>(required_layers.size()),
			.ppEnabledLayerNames = required_layers.data(),
			.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions.size()),
			.ppEnabledExtensionNames = required_extensions.data()
		};

		m_instance = vk::raii::Instance(m_context, create_info);
	}

	void Vulkan::print_extensions_and_layers(std::span<const char* const> required_extensions,
		std::span<const vk::ExtensionProperties> vulkan_extensions,
		std::span<const char* const> required_layers,
		std::span<const vk::LayerProperties> vulkan_layers) const
	{
		std::println("[Debug]: Available Vulkan extensions:");
		for (const auto& extension : vulkan_extensions)
			std::println("\t - {}", std::string_view { extension.extensionName });

		std::println("\n[Debug]: Required GLFW extensions:");
		for (const auto& extension : required_extensions)
			std::println("\t - {}", std::string_view { extension });

		std::println("\n[Debug]: Available Vulkan layers:");
		for (const auto& layer : vulkan_layers)
			std::println("\t - {}", std::string_view { layer.layerName });

		std::println("\n[Debug]: Required Vulkan layers:");
		for (const auto& layer : required_layers)
			std::println("\t - {}", std::string_view { layer });
	}

	void Vulkan::check_required_extensions_and_layers(std::span<const char* const> required_extensions,
		std::span<const vk::ExtensionProperties> vulkan_extensions,
		std::span<const char* const> required_layers,
		std::span<const vk::LayerProperties> vulkan_layers) const
	{
		for (const auto& extension : required_extensions)
		{
			const bool missing { std::ranges::none_of(vulkan_extensions,
				[extension](const auto& vulkan_extension)
				{
					return std::string_view { vulkan_extension.extensionName } == extension;
				}) };

			if (missing)
			{
				throw std::runtime_error(std::format(
					"\n[Error]: The {} extension required by GLFW is not supported.\n",
					extension));
			}
		}

		for (const auto& layer : required_layers)
		{
			const bool missing { std::ranges::none_of(vulkan_layers,
				[layer](const auto& vulkan_layer)
				{
					return std::string_view { vulkan_layer.layerName } == layer;
				}) };

			if (missing)
			{
				throw std::runtime_error(std::format(
					"\n[Error]: The {} layer required by Vulkan is not supported.\n",
					layer));
			}
		}
	}

	std::vector<const char*> get_required_extensions()
	{
		std::uint32_t required_extension_count {};
		const auto required_extensions { glfwGetRequiredInstanceExtensions(&required_extension_count) };

		std::vector extensions(required_extensions, required_extensions + required_extension_count);
		if (constants::g_enable_validation_layers)
			extensions.push_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	std::vector<const char*> get_required_layers()
	{
		std::vector<const char*> required_layers {};
		if (constants::g_enable_validation_layers)
		{
			required_layers.assign(constants::g_required_layers.begin(),
				constants::g_required_layers.end());
		}

		return required_layers;
	}

	VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT,
		vk::DebugUtilsMessageTypeFlagsEXT type,
		const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*)
	{
		std::cerr << std::format("\n[Debug]: Validation layer:\n\t- Type: {}"
			"\n\t - Message: {}\n", vk::to_string(type), callback_data->pMessage);

		return vk::False;
	}

	void Vulkan::setup_debug_messenger()
	{
		if (!constants::g_enable_validation_layers) return;

		constexpr vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

		constexpr vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger {
			.messageSeverity = severity_flags,
			.messageType = message_type_flags,
			.pfnUserCallback = &diaxx::debug_callback
		};

		m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(debug_utils_messenger);
	}

	void Vulkan::pick_physical_device()
	{
		const auto devices { m_instance.enumeratePhysicalDevices() };

		const auto device_iterator { std::ranges::find_if(devices,
			[&](const auto& device)
			{
				// Each family supports specific operations like graphics, compute, transfer...
				const auto queue_families { device.getQueueFamilyProperties() };
				bool is_suitable { device.getProperties().apiVersion >= VK_API_VERSION_1_3 };

				// Find the first queue family that supports graphics commands
				const auto qfp_iterator { std::ranges::find_if(queue_families,
					[](const auto& qfp)
					{
						 return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
							 static_cast<vk::QueueFlags>(0);
					})};

				is_suitable = is_suitable && (qfp_iterator != queue_families.end());
				const auto extensions { device.enumerateDeviceExtensionProperties() };
				bool found { true };

				for (const auto& device_extension : constants::g_device_extensions)
				{
					const auto extension_iterator { std::ranges::find_if(extensions,
						[device_extension](const auto& extension)
						{
							return std::string_view { extension.extensionName } == device_extension;
						})};
					found = found && (extension_iterator != extensions.end());
				}

				is_suitable = is_suitable && found;
				if (is_suitable)
					m_physical_device = device;

				return is_suitable;
			}) };

		if (device_iterator == devices.end())
			throw std::runtime_error("\n[Error]: No suitable GPU could be found.\n");
	}

	void Vulkan::main_loop()
	{
		while (!glfwWindowShouldClose(m_window))
			glfwPollEvents();
	}
}