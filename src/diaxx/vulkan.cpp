#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

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
		// It enables Vulkan debug messages
		setup_debug_messenger();
	}

	void Vulkan::create_instance()
	{
		// Optional structure with information about the application
		constexpr vk::ApplicationInfo application_info("Diaxx", VK_MAKE_VERSION(1, 0, 0),
			"Diaxx Engine", VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion14);

		// Validation layers specified by the developer
		std::vector<const char*> required_layers {};
		if (Constants::g_enable_validation_layers)
		{
			required_layers.assign(Constants::g_validation_layers.begin(),
				Constants::g_validation_layers.end());
		}

		// Verify that the specified validation layers are compatible with Vulkan validation layers
		const auto vulkan_layers { m_context.enumerateInstanceLayerProperties() };
		for (std::size_t i {}; i < required_layers.size(); ++i)
		{
			if (std::ranges::none_of(vulkan_layers,
				[required_layer = required_layers[i]](auto const& vulkan_layer)
				{ return !std::strcmp(required_layer, vulkan_layer.layerName); }))
			{
				throw std::runtime_error(std::format(
					"[Error]: The validation layer {} is not supported.", required_layers[i]));
			}
		}

		// GLFW asks Vulkan which instance extensions it needs
		const auto glfw_extensions { get_required_extensions() };

		// Check that GLFW instance extensions are compatible with Vulkan instance extensions
		const auto vulkan_extensions { m_context.enumerateInstanceExtensionProperties() };
		for (const auto& glfw_extension : glfw_extensions)
		{
			if (std::ranges::none_of(vulkan_extensions,
				[glfw_extension](auto const& vulkan_extension)
				{ return !std::strcmp(glfw_extension, vulkan_extension.extensionName); }))
			{
				throw std::runtime_error(std::format(
					"[Error]: The GLFW instance extension {} is not supported.", glfw_extension));
			}
		}

		// Tell Vulkan which global extensions and validation layers we will use
		const vk::InstanceCreateInfo create_info({}, &application_info,
			static_cast<std::uint32_t>(required_layers.size()), required_layers.data(),
			static_cast<std::uint32_t>(glfw_extensions.size()), glfw_extensions.data());

		// RAII call to create the Vulkan instance
		m_instance = vk::raii::Instance(m_context, create_info);
	}

	std::vector<const char*> Vulkan::get_required_extensions()
	{
		std::uint32_t glfw_extension_count {};
		const auto glfw_extensions { glfwGetRequiredInstanceExtensions(&glfw_extension_count) };

		std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		if (Constants::g_enable_validation_layers)
			extensions.push_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	// Print validation layer messages from Vulkan
	VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::debug_callback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT,
		vk::DebugUtilsMessageTypeFlagsEXT message_type,
		const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*)
	{
		std::print(std::cerr, "[Debug]: Validation Layer\n\t- Type: {}\n\t- Message: {}\n",
			vk::to_string(message_type), callback_data->pMessage);

		return vk::False;
	}

	void Vulkan::setup_debug_messenger()
	{
		if (!Constants::g_enable_validation_layers) return;

		constexpr vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

		constexpr vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger({}, severity_flags,
			message_type_flags, &debug_callback);

		m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(debug_utils_messenger);
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