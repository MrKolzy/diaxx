#include "diaxx/constants.hpp"
#include "diaxx/vulkan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>

namespace diaxx
{
	GLFWContext::GLFWContext()
	{
		glfwSetErrorCallback([](int code, const char* message) {
			std::cerr << std::format("\n[Error]: GLFW {}, {}\n", code, message); });

		if (glfwInit() != GLFW_TRUE)
			throw std::runtime_error("\n[Error]: GLFW could not be initialized.\n");
	}

	void Vulkan::run()
	{
		initialize_window();
		initialize_vulkan();
		main_loop();
	}

	void Vulkan::initialize_window()
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Disable OpenGL
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_window.reset(glfwCreateWindow(constants::g_width, constants::g_height, "Diaxx", nullptr, nullptr));

		if (!m_window)
			throw std::runtime_error("\n[Error]: The glfwCreateWindow function failed.\n");
	}

	void Vulkan::initialize_vulkan()
	{
		// Enables communication between the application and Vulkan
		create_instance();
		// Connects the GLFW window with Vulkan to define where rendered frames will be presented
		create_surface();
		// Select a suitable GPU (physical device) that supports the required Vulkan features
		pick_physical_device();
		// Handle used to talk to the GPU
		create_logical_device();
		// Queue of images that Vulkan will render and present on the window surface
		create_swap_chain();
		// Prepares the swap chain images so the GPU can actually use them
		create_image_views();
		// Defines how the GPU process vertices and fragments into pixels on the screen
		create_graphics_pipeline();
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

		vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger {};
		void* p_next {};
		if (constants::g_enable_validation_layers)
		{
			debug_utils_messenger = vk::DebugUtilsMessengerCreateInfoEXT {
				.messageSeverity = constants::severity_flags,
				.messageType = constants::message_type_flags,
				.pfnUserCallback = &Vulkan::debug_callback
			};
			p_next = &debug_utils_messenger;
		}

		// Structure with the information Vulkan needs
		const vk::InstanceCreateInfo create_info {
			.pNext = p_next,
			.pApplicationInfo = &application_info,
			.enabledLayerCount = static_cast<std::uint32_t>(required_layers.size()),
			.ppEnabledLayerNames = required_layers.data(),
			.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions.size()),
			.ppEnabledExtensionNames = required_extensions.data()
		};

		m_instance = vk::raii::Instance(m_context, create_info);

		// Enables Vulkan debug messages for validation
		if (constants::g_enable_validation_layers)
			m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(debug_utils_messenger);
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
					"\n[Error]: The {} extension required by GLFW is not supported.\n", extension));
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
					"\n[Error]: The {} layer required by Vulkan is not supported.\n", layer));
			}
		}
	}

	std::vector<const char*> Vulkan::get_required_extensions()
	{
		std::uint32_t required_extension_count {};
		const auto required_extensions { glfwGetRequiredInstanceExtensions(&required_extension_count) };

		std::vector extensions(required_extensions, required_extensions + required_extension_count);
		if (constants::g_enable_validation_layers)
			extensions.push_back(vk::EXTDebugUtilsExtensionName);

		return extensions;
	}

	std::vector<const char*> Vulkan::get_required_layers()
	{
		std::vector<const char*> required_layers {};
		if (constants::g_enable_validation_layers)
			required_layers.assign(constants::g_required_layers.begin(), constants::g_required_layers.end());

		return required_layers;
	}

	VKAPI_ATTR vk::Bool32 VKAPI_CALL Vulkan::debug_callback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT,
		vk::DebugUtilsMessageTypeFlagsEXT,
		const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*)
	{
		std::cerr << std::format("\n[Debug]: {}", callback_data->pMessage);

		return vk::False;
	}

	void Vulkan::create_surface()
	{
		VkSurfaceKHR surface {};
		if (glfwCreateWindowSurface(*m_instance, m_window.get(), nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("\n[Error]: The window surface could not be created.\n");

		m_surface = vk::raii::SurfaceKHR(m_instance, surface);
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
						 return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags {};
					})};

				if (qfp_iterator == queue_families.end())
					return false;

				// Queue family index that supports graphics commands (drawing)
				const auto graphics_index { static_cast<std::uint32_t>(
					std::distance(queue_families.begin(), qfp_iterator)) };

				const auto extensions { device.enumerateDeviceExtensionProperties() };
				bool found { true };

				// Features that the GPU must explicitly advertise and support
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

				// Queue family index that supports presentation (show images on a surface)
				std::uint32_t present_index { static_cast<std::uint32_t>(queue_families.size()) };
				if (device.getSurfaceSupportKHR(graphics_index, *m_surface))
					present_index = graphics_index;
				else
				{
					for (std::uint32_t i {}; i < queue_families.size(); ++i)
					{
						if (device.getSurfaceSupportKHR(i, *m_surface))
						{
							present_index = i;
							break;
						}
					}
				}
				is_suitable = is_suitable && (present_index != queue_families.size());

				if (is_suitable)
				{
					m_physical_device = device;
					m_graphics_queue_family_index = graphics_index;
					m_present_queue_family_index = present_index;
				}

				return is_suitable;
			}) };

		if (device_iterator == devices.end())
			throw std::runtime_error("\n[Error]: No suitable GPU could be found.\n");
	}

	void Vulkan::create_logical_device()
	{
		const uint32_t graphics_index = m_graphics_queue_family_index;
		const uint32_t present_index = m_present_queue_family_index;

		constexpr float queue_priority { 1.0f };
		std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos {};

		device_queue_create_infos.push_back(vk::DeviceQueueCreateInfo {
			.queueFamilyIndex = graphics_index,
			.queueCount = 1,
			.pQueuePriorities = &queue_priority });

		if (present_index != graphics_index)
		{
			device_queue_create_infos.push_back(vk::DeviceQueueCreateInfo {
			.queueFamilyIndex = present_index,
			.queueCount = 1,
			.pQueuePriorities = &queue_priority });
		}

		const vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain {
				{},
				{.dynamicRendering = true },
				{.extendedDynamicState = true }
		};

		const vk::DeviceCreateInfo device_create_info {
			.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = static_cast<std::uint32_t>(device_queue_create_infos.size()),
			.pQueueCreateInfos = device_queue_create_infos.data(),
			.enabledExtensionCount = static_cast<std::uint32_t>(constants::g_device_extensions.size()),
			.ppEnabledExtensionNames = constants::g_device_extensions.data()
		};

		m_device = vk::raii::Device(m_physical_device, device_create_info);
		m_graphics_queue = vk::raii::Queue(m_device, graphics_index, 0);
		m_present_queue = vk::raii::Queue(m_device, present_index, 0);
	}

	void Vulkan::create_swap_chain()
	{
		const auto surface_capabilities { m_physical_device.getSurfaceCapabilitiesKHR(m_surface) };
		const auto swap_chain_surface_format { choose_swap_surface_format(
			m_physical_device.getSurfaceFormatsKHR(m_surface)) };

		// Tells Vulkan the resolution of the images in the swapchain
		const auto swap_chain_extent { choose_swap_extent(surface_capabilities) };

		// How many images we keep in the swapchain for Vulkan to render and present
		std::uint32_t image_count { surface_capabilities.minImageCount + 1 };
		if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
			image_count = surface_capabilities.maxImageCount;

		vk::SwapchainCreateInfoKHR swap_chain_create_info {
			.surface = m_surface,
			.minImageCount = image_count,
			.imageFormat = swap_chain_surface_format.format,
			.imageColorSpace = swap_chain_surface_format.colorSpace,
			.imageExtent = swap_chain_extent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.preTransform = surface_capabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = choose_swap_present_mode(m_physical_device.getSurfacePresentModesKHR(m_surface)),
			.clipped = vk::True,
			.oldSwapchain = nullptr
		};

		const std::array<uint32_t, 2> queue_family_indices { m_graphics_queue_family_index, m_present_queue_family_index };

		if (m_graphics_queue_family_index != m_present_queue_family_index)
		{
			swap_chain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
			swap_chain_create_info.queueFamilyIndexCount = 2;
			swap_chain_create_info.pQueueFamilyIndices = queue_family_indices.data();
		}
		else
			swap_chain_create_info.imageSharingMode = vk::SharingMode::eExclusive;

		m_swap_chain = vk::raii::SwapchainKHR(m_device, swap_chain_create_info);

		m_swap_chain_images = m_swap_chain.getImages();
		m_swap_chain_image_format = swap_chain_surface_format.format;
		m_swap_chain_extent = swap_chain_extent;
	}

	vk::SurfaceFormatKHR Vulkan::choose_swap_surface_format(
		const std::vector<vk::SurfaceFormatKHR>& available_formats)
	{
		for (const auto& available_format : available_formats)
		{
			if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
				available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return available_format;
			}
		}

		return available_formats[0];
	}

	vk::PresentModeKHR Vulkan::choose_swap_present_mode(
		const std::vector<vk::PresentModeKHR>& available_present_modes)
	{
		for (const auto& available_present_mode : available_present_modes)
		{
			if (available_present_mode == vk::PresentModeKHR::eMailbox)
				return available_present_mode;
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Vulkan::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
			return capabilities.currentExtent;

		int width {};
		int height {};
		glfwGetFramebufferSize(m_window.get(), &width, &height);

		// Makes sure the swapchain image size matches the window size
		return {
			std::clamp<std::uint32_t>(static_cast<std::uint32_t>(width), capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width),
			std::clamp<std::uint32_t>(static_cast<std::uint32_t>(height), capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height) };
	}

	void Vulkan::create_image_views()
	{
		m_swap_chain_image_views.clear();
		m_swap_chain_image_views.reserve(m_swap_chain_images.size());

		vk::ImageViewCreateInfo image_view_create_info {
			.viewType = vk::ImageViewType::e2D,
			.format = m_swap_chain_image_format,
			.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		};

		for (const auto& image : m_swap_chain_images)
		{
			image_view_create_info.image = image;
			m_swap_chain_image_views.emplace_back(m_device, image_view_create_info);
		}
	}

	void Vulkan::create_graphics_pipeline()
	{
		const vk::raii::ShaderModule shader_module { create_shader_module(read_file("shaders/shader.spv")) };

		const vk::PipelineShaderStageCreateInfo vertex_shader_stage_info {
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = shader_module,
			.pName = "vertMain"
		};

		const vk::PipelineShaderStageCreateInfo fragment_shader_stage_info {
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = shader_module,
			.pName = "fragMain"
		};

		const vk::PipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info,
			fragment_shader_stage_info };

		const std::vector dynamic_states { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		const vk::PipelineDynamicStateCreateInfo dynamic_state {
			.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size()),
			.pDynamicStates = dynamic_states.data()
		};

		constexpr vk::PipelineVertexInputStateCreateInfo vertex_input_info {};

		constexpr vk::PipelineInputAssemblyStateCreateInfo input_assembly {
			.topology = vk::PrimitiveTopology::eTriangleList
		};

		constexpr vk::PipelineViewportStateCreateInfo viewport_state {
			.viewportCount = 1,
			.scissorCount = 1
		};

		constexpr vk::PipelineRasterizationStateCreateInfo rasterizer {
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eClockwise,
			.depthBiasEnable = vk::False,
			.depthBiasSlopeFactor = 1.0f,
			.lineWidth = 1.0f
		};

		constexpr vk::PipelineMultisampleStateCreateInfo multisampling {
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False
		};

		constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment {
			.blendEnable = vk::True,
			.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
			.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
			.colorBlendOp = vk::BlendOp::eAdd,
			.srcAlphaBlendFactor = vk::BlendFactor::eOne,
			.dstAlphaBlendFactor = vk::BlendFactor::eZero,
			.alphaBlendOp = vk::BlendOp::eAdd,
			.colorWriteMask = vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA,
		};

		const vk::PipelineColorBlendStateCreateInfo color_blending {
			.logicOpEnable = vk::False,
			.logicOp = vk::LogicOp::eCopy,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment
		};

		constexpr vk::PipelineLayoutCreateInfo pipeline_layout_info {
			.setLayoutCount = 0,
			.pushConstantRangeCount = 0
		};

		m_pipeline_layout = vk::raii::PipelineLayout(m_device, pipeline_layout_info);
	}

	std::vector<char> Vulkan::read_file(const std::string& file_name)
	{
		std::ifstream file { file_name, std::ios::ate | std::ios::binary };
		if (!file.is_open())
			throw std::runtime_error("\n[Error]: Failed to open the file.\n");

		const auto file_size { static_cast<std::size_t>(file.tellg()) };
		std::vector<char> buffer(file_size);

		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();

		return buffer;
	}

	vk::raii::ShaderModule Vulkan::create_shader_module(const std::vector<char>& code) const
	{
		const vk::ShaderModuleCreateInfo create_info {
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const std::uint32_t*>(code.data())
		};

		vk::raii::ShaderModule shader_module { m_device, create_info };

		return shader_module;
	}

	void Vulkan::main_loop()
	{
		while (!glfwWindowShouldClose(m_window.get()))
			glfwWaitEventsTimeout(1.0 / 60.0);
	}
}