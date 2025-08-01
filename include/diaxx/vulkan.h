#pragma once

#define  GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace internal
{
    struct QueueFamilyIndices
    {
        std::optional<std::uint32_t> m_graphics_family {};
        std::optional<std::uint32_t> m_present_family  {};

        bool is_complete() const { return m_graphics_family.has_value() && m_present_family.has_value(); }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        m_capabilities  {};
        std::vector<VkSurfaceFormatKHR> m_formats       {};
        std::vector<VkPresentModeKHR>   m_present_modes {};
    };
}

class Vulkan
{
public:
    Vulkan () = default;
    ~Vulkan();

    // Delete copy constructor and copy assignment
    Vulkan(const Vulkan&)            = delete;
    Vulkan& operator=(const Vulkan&) = delete;

    // Delete move constructor and move assignment
    Vulkan(Vulkan&&)                 = delete;
    Vulkan& operator=(Vulkan&&)      = delete;

    void run();

private:
    void initialize_window       ();

    void initialize_vulkan       ();
    void create_instance         (bool show_extensions);
    void setup_debug_messenger   ();
    void create_surface          ();
    void pick_physical_device    ();
    void create_logical_device   ();
    void create_swap_chain       ();
    void create_image_views      ();
    void create_render_pass      ();
    void create_graphics_pipeline();
    void create_frame_buffers    ();
    void create_command_pool     ();
    void create_command_buffer   ();

    // draw_frame
    void record_command_buffer(VkCommandBuffer command_buffer, std::uint32_t image_index) const;

    // create_graphics_pipeline
    static std::vector<char> read_file(const std::string& file_name);
    VkShaderModule create_shader_module(const std::vector<char>& code) const;

    // create_swap_chain
    internal::SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    // create_logical_device
    internal::QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;

    // pick_physical_device
    bool is_device_suitable(VkPhysicalDevice device);
    bool check_device_extension_support(VkPhysicalDevice device);
    
    // create_instance
    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);  
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
    void show_supported_extensions(std::uint32_t glfw_extension_count, const char** glfw_extension_names);

    void main_loop();

    void cleanup();

    GLFWwindow*                m_window                   {};
    VkInstance                 m_instance                 {};
    VkDebugUtilsMessengerEXT   m_debug_messenger          {};
    VkPhysicalDevice           m_physical_device          {};
    VkDevice                   m_device                   {};
    VkQueue                    m_graphics_queue           {};
    VkSurfaceKHR               m_surface                  {};
    VkQueue                    m_present_queue            {};
    VkSwapchainKHR             m_swap_chain               {};
    std::vector<VkImage>       m_swap_chain_images        {};
    VkFormat                   m_swap_chain_image_format  {};
    VkExtent2D                 m_swap_chain_extent        {};
    std::vector<VkImageView>   m_swap_chain_image_views   {};
    VkPipelineLayout           m_pipeline_layout          {};
    VkRenderPass               m_render_pass              {};
    VkPipeline                 m_graphics_pipeline        {};
    std::vector<VkFramebuffer> m_swap_chain_frame_buffers {};
    VkCommandPool              m_command_pool             {};
    VkCommandBuffer            m_command_buffer           {};
};