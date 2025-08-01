#include "diaxx/constants.h"
#include "diaxx/vulkan.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <print>
#include <set>
#include <stdexcept>

namespace
{
	// setup_debug_messenger
	static VkResult create_debug_utils_messenger_ext(VkInstance instance,
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

	// cleanup
	static void destroy_debug_utils_messenger_ext(VkInstance instance,
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

void Vulkan::run()
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
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
	create_image_views();
	create_render_pass();
	create_graphics_pipeline();
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
	const char** glfw_extension_names { required_extensions.data() };

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
		create_info.enabledLayerCount   = static_cast<std::uint32_t>(
			constants::g_validation_layers.size());
		create_info.ppEnabledLayerNames = constants::g_validation_layers.data();

		populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = &debug_create_info;
	}

	// Retrieve a list of supported extensions before creating an instance
	if (show_extensions)
	{
		show_supported_extensions(glfw_extension_count, glfw_extension_names);
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

void Vulkan::create_surface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create window surface.");
}

void Vulkan::pick_physical_device()
{
	std::uint32_t device_count {};
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

	if (!device_count)
		throw std::runtime_error("[Error]: Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	for (const auto& device : devices)
	{
		if (is_device_suitable(device))
		{
			m_physical_device = device;
			break;
		}
	}

	if (m_physical_device == VK_NULL_HANDLE)
		throw std::runtime_error("[Error]: Failed to find a suitable GPU.");
}

void Vulkan::create_logical_device()
{
	const internal::QueueFamilyIndices indices { find_queue_families(m_physical_device) };

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos {};
	const std::set<std::uint32_t> unique_queue_families { indices.m_graphics_family.value(),
		indices.m_present_family.value() };

	const float queue_priority { 1.0f };
	for (std::uint32_t queue_family : unique_queue_families)
	{
		const VkDeviceQueueCreateInfo queue_create_info {
			.sType            { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO },
			.queueFamilyIndex { queue_family                               },
			.queueCount       { 1                                          },
			.pQueuePriorities { &queue_priority                            },
		};
		queue_create_infos.push_back(queue_create_info);
	}

	const VkPhysicalDeviceFeatures device_features {};

	VkDeviceCreateInfo create_info {
		.sType                   { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO                              },
		.queueCreateInfoCount    { static_cast<std::uint32_t>(queue_create_infos.size())             },
		.pQueueCreateInfos       { queue_create_infos.data()                                         },
		.enabledExtensionCount   { static_cast<std::uint32_t>(constants::g_device_extensions.size()) },
		.ppEnabledExtensionNames { constants::g_device_extensions.data()                             },
		.pEnabledFeatures        { &device_features                                                  }
	};

	if (constants::g_enable_validation_layers)
	{
		create_info.enabledLayerCount   = static_cast<std::uint32_t>(constants::g_validation_layers.size());
		create_info.ppEnabledLayerNames = constants::g_validation_layers.data();
	}

	if (vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create logical device.");

	vkGetDeviceQueue(m_device, indices.m_graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.m_present_family.value() , 0, &m_present_queue );
}

void Vulkan::create_swap_chain()
{
	const internal::SwapChainSupportDetails swap_chain_support { query_swap_chain_support(m_physical_device) };

	if (swap_chain_support.m_formats.empty())
		throw std::runtime_error("[Error]: No supported surface formats found.");

	if (swap_chain_support.m_present_modes.empty())
		throw std::runtime_error("[Error]: No supported present modes found.");

	const VkSurfaceFormatKHR surface_format { choose_swap_surface_format(swap_chain_support.m_formats)     };
	const VkPresentModeKHR   present_mode   { choose_swap_present_mode(swap_chain_support.m_present_modes) };
	const VkExtent2D         extent         { choose_swap_extent(swap_chain_support.m_capabilities)        };

	std::uint32_t image_count { swap_chain_support.m_capabilities.minImageCount + 1 };

	if (swap_chain_support.m_capabilities.maxImageCount > 0 &&
		image_count > swap_chain_support.m_capabilities.maxImageCount)
		image_count = swap_chain_support.m_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info {
		.sType            { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR        },
		.surface          { m_surface                                          },
		.minImageCount    { image_count                                        },
		.imageFormat      { surface_format.format                              },
		.imageColorSpace  { surface_format.colorSpace                          },
		.imageExtent      { extent                                             },
		.imageArrayLayers { 1                                                  },
		.imageUsage       { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                },
		.preTransform     { swap_chain_support.m_capabilities.currentTransform },
		.compositeAlpha   { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR                  },
		.presentMode      { present_mode                                       },
		.clipped          { VK_TRUE                                            },
		.oldSwapchain     { VK_NULL_HANDLE                                     }
	};

	const internal::QueueFamilyIndices indices { find_queue_families(m_physical_device) };
	const std::uint32_t queue_family_indices[] { indices.m_graphics_family.value(),
		indices.m_present_family.value() };

	if (indices.m_graphics_family != indices.m_present_family)
	{
		create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices   = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices   = nullptr;
	}

	if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swap_chain) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create swap chain.");

	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, nullptr);
	m_swap_chain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count, m_swap_chain_images.data());

	m_swap_chain_image_format = surface_format.format;
	m_swap_chain_extent       = extent;
}

void Vulkan::create_image_views()
{
	m_swap_chain_image_views.resize(m_swap_chain_images.size());

	for (std::size_t i {}; i < m_swap_chain_images.size(); ++i)
	{
		VkImageViewCreateInfo create_info {
			.sType    { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO },
			.image    { m_swap_chain_images[i]                   },
			.viewType { VK_IMAGE_VIEW_TYPE_2D                    },
			.format   { m_swap_chain_image_format                }
		};

		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel   = 0;
		create_info.subresourceRange.levelCount     = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(m_device, &create_info, nullptr, &m_swap_chain_image_views[i]) != VK_SUCCESS)
			throw std::runtime_error("[Error]: Failed to create image views.");
	}
}

void Vulkan::create_render_pass()
{
	const VkAttachmentDescription color_attachment {
		.format         { m_swap_chain_image_format        },
		.samples        { VK_SAMPLE_COUNT_1_BIT            },
		.loadOp         { VK_ATTACHMENT_LOAD_OP_CLEAR      },
		.storeOp        { VK_ATTACHMENT_STORE_OP_STORE     },
		.stencilLoadOp  { VK_ATTACHMENT_LOAD_OP_DONT_CARE  },
		.stencilStoreOp { VK_ATTACHMENT_STORE_OP_DONT_CARE },
		.initialLayout  { VK_IMAGE_LAYOUT_UNDEFINED        },
		.finalLayout    { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR  }
	};

	const VkAttachmentReference color_attachment_reference {
		.attachment { 0                                        },
		.layout     { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
	};

	const VkSubpassDescription subpass {
		.pipelineBindPoint    { VK_PIPELINE_BIND_POINT_GRAPHICS },
		.colorAttachmentCount { 1                               },
		.pColorAttachments    { &color_attachment_reference     }
	};

	const VkRenderPassCreateInfo render_pass_info {
		.sType           { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO },
		.attachmentCount { 1                                         },
		.pAttachments    { &color_attachment                         },
		.subpassCount    { 1                                         },
		.pSubpasses      { &subpass                                  }
	};

	if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create render pass.");
}

void Vulkan::create_graphics_pipeline()
{
	const auto vert_shader_code { read_file("shaders/vert.spv") };
	const auto frag_shader_code { read_file("shaders/frag.spv") };

	const VkShaderModule vert_shader_module { create_shader_module(vert_shader_code) };
	const VkShaderModule frag_shader_module { create_shader_module(frag_shader_code) };

	const VkPipelineShaderStageCreateInfo vert_shaders_stage_info {
		.sType  { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		.stage  { VK_SHADER_STAGE_VERTEX_BIT                          },
		.module { vert_shader_module                                  },
		.pName  { "main"                                              }
	};

	const VkPipelineShaderStageCreateInfo frag_shaders_stage_info {
		.sType  { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		.stage  { VK_SHADER_STAGE_FRAGMENT_BIT                        },
		.module { frag_shader_module                                  },
		.pName  { "main"                                              }
	};

	const VkPipelineShaderStageCreateInfo shader_stages[] { vert_shaders_stage_info,
		frag_shaders_stage_info };

	const std::vector<VkDynamicState> dynamic_states { VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR };

	const VkPipelineDynamicStateCreateInfo dynamic_state {
		.sType             { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO },
		.dynamicStateCount { static_cast<std::uint32_t>(dynamic_states.size())    },
		.pDynamicStates    { dynamic_states.data()                                }
	};

	const VkPipelineVertexInputStateCreateInfo vertex_input_info {
		.sType                           { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO },
		.vertexBindingDescriptionCount   { 0                                                         },
		.pVertexBindingDescriptions      { nullptr                                                   },
		.vertexAttributeDescriptionCount { 0                                                         },
		.pVertexAttributeDescriptions    { nullptr                                                   }
	};

	const VkPipelineInputAssemblyStateCreateInfo input_assembly {
		.sType                  { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO },
		.topology               { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST                         },
		.primitiveRestartEnable { VK_FALSE                                                    }
	};

	const VkViewport viewport {
		.x        { 0.0f                                           },
		.y        { 0.0f                                           },
		.width    { static_cast<float>(m_swap_chain_extent.width)  },
		.height   { static_cast<float>(m_swap_chain_extent.height) },
		.minDepth { 0.0f                                           },
		.maxDepth { 1.0f                                           },
	};

	const VkRect2D scissor { .offset { 0, 0 }, .extent { m_swap_chain_extent } };

	const VkPipelineViewportStateCreateInfo viewport_state {
		.sType         { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO },
		.viewportCount { 1                                                     },
		.pViewports    { &viewport                                             },
		.scissorCount  { 1                                                     },
		.pScissors     { &scissor                                              }
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer {
		.sType                   { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO },
		.depthClampEnable        { VK_FALSE                                                   },
		.rasterizerDiscardEnable { VK_FALSE                                                   },
		.polygonMode             { VK_POLYGON_MODE_FILL                                       },
		.cullMode                { VK_CULL_MODE_BACK_BIT                                      },
		.frontFace               { VK_FRONT_FACE_CLOCKWISE                                    },
		.depthBiasEnable         { VK_FALSE                                                   },
		.depthBiasConstantFactor { 0.0f                                                       },
		.depthBiasClamp          { 0.0f                                                       },
		.depthBiasSlopeFactor    { 0.0f                                                       },
		.lineWidth               { 1.0f                                                       },
	};

	const VkPipelineMultisampleStateCreateInfo multisampling {
		.sType                 { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO },
		.rasterizationSamples  { VK_SAMPLE_COUNT_1_BIT                                    },
		.sampleShadingEnable   { VK_FALSE                                                 },
		.minSampleShading      { 1.0f                                                     },
		.pSampleMask           { nullptr                                                  },
		.alphaToCoverageEnable { VK_FALSE                                                 },
		.alphaToOneEnable      { VK_FALSE                                                 }
	};

	const VkPipelineColorBlendAttachmentState color_blend_attachment {
		.blendEnable         { VK_TRUE                             },
		.srcColorBlendFactor { VK_BLEND_FACTOR_SRC_ALPHA           },
		.dstColorBlendFactor { VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
		.colorBlendOp        { VK_BLEND_OP_ADD                     },
		.srcAlphaBlendFactor { VK_BLEND_FACTOR_ONE                 },
		.dstAlphaBlendFactor { VK_BLEND_FACTOR_ZERO                },
		.alphaBlendOp        { VK_BLEND_OP_ADD                     },
		.colorWriteMask      { VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT        },
	};

	VkPipelineColorBlendStateCreateInfo color_blending {
		.sType           { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO },
		.logicOpEnable   { VK_FALSE                                                 },
		.logicOp         { VK_LOGIC_OP_COPY                                         },
		.attachmentCount { 1                                                        },
		.pAttachments    { &color_blend_attachment                                  },
	};

	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	const VkPipelineLayoutCreateInfo pipeline_layout_info {
		.sType                  { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO },
		.setLayoutCount         { 0                                             },
		.pSetLayouts            { nullptr                                       },
		.pushConstantRangeCount { 0                                             },
		.pPushConstantRanges    { nullptr                                       }
	};

	if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create pipeline layout.");

	vkDestroyShaderModule(m_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
}

std::vector<char> Vulkan::read_file(const std::string& file_name)
{
	std::ifstream file(file_name, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("[Error]: Failed to open file.");

	const std::size_t file_size { static_cast<std::size_t>(file.tellg()) };
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(file_size));

	file.close();

	return buffer;
}

VkShaderModule Vulkan::create_shader_module(const std::vector<char>& code) const
{
	const VkShaderModuleCreateInfo create_info {
		.sType    { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO         },
		.codeSize { code.size()                                         },
		.pCode    { reinterpret_cast<const std::uint32_t*>(code.data()) }
	};

	VkShaderModule shader_module {};
	if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
		throw std::runtime_error("[Error]: Failed to create shader module");

	return shader_module;
}

internal::SwapChainSupportDetails Vulkan::query_swap_chain_support(VkPhysicalDevice device) const
{
	internal::SwapChainSupportDetails details {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.m_capabilities);

	std::uint32_t format_count {};
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);

	if (format_count)
	{
		details.m_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.m_formats.data());
	}

	std::uint32_t present_mode_count {};
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);

	if (present_mode_count)
	{
		details.m_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count,
			details.m_present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR Vulkan::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& available_format : available_formats)
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;

	return available_formats[0];
}

VkPresentModeKHR Vulkan::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes)
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return available_present_mode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		int width  {};
		int height {};
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actual_extent { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height) };

		actual_extent.width  = std::clamp(actual_extent.width, capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

internal::QueueFamilyIndices Vulkan::find_queue_families(VkPhysicalDevice device) const
{
	internal::QueueFamilyIndices indices {};

	std::uint32_t queue_family_count {};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	std::uint32_t i {};
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.m_graphics_family = i;

		VkBool32 present_support { false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);

		if (present_support)
			indices.m_present_family = i;

		if (indices.is_complete())
			break;

		++i;
	}

	return indices;
}

bool Vulkan::is_device_suitable(VkPhysicalDevice device)
{
	const internal::QueueFamilyIndices indices { find_queue_families(device) };
	
	const bool extensions_supported { check_device_extension_support(device) };

	bool swap_chain_adequate {};
	if (extensions_supported)
	{
		const internal::SwapChainSupportDetails swap_chain_support { query_swap_chain_support(device) };
		swap_chain_adequate = !swap_chain_support.m_formats.empty()
			&& !swap_chain_support.m_present_modes.empty();
	}

	return indices.is_complete() && extensions_supported && swap_chain_adequate;
}

bool Vulkan::check_device_extension_support(VkPhysicalDevice device)
{
	std::uint32_t extension_count {};
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(constants::g_device_extensions.begin(),
		constants::g_device_extensions.end());

	for (const auto& extension : available_extensions)
		required_extensions.erase(extension.extensionName);

	return required_extensions.empty();
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

void Vulkan::show_supported_extensions(std::uint32_t glfw_extension_count, const char** glfw_extension_names)
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

void Vulkan::main_loop()
{
	while (!glfwWindowShouldClose(m_window))
		glfwPollEvents();
}

void Vulkan::cleanup()
{
	if (m_pipeline_layout)
	{
		vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
		m_pipeline_layout = nullptr;
	}

	if (m_render_pass)
	{
		vkDestroyRenderPass(m_device, m_render_pass, nullptr);
		m_render_pass = nullptr;
	}

	for (const auto image_view : m_swap_chain_image_views) {
		if (image_view)
			vkDestroyImageView(m_device, image_view, nullptr);
	}
	m_swap_chain_image_views.clear();

	if (m_swap_chain)
	{
		vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
		m_swap_chain = nullptr;
	}

	if (m_device)
	{
		vkDestroyDevice(m_device, nullptr);
		m_device = nullptr;
	}

	if (constants::g_enable_validation_layers)
	{
		destroy_debug_utils_messenger_ext(m_instance, m_debug_messenger, nullptr);
		m_debug_messenger = nullptr;
	}
	
	if (m_surface)
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = nullptr;
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