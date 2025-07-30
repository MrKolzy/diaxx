#pragma once

#define  GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <cstdint>
#include <optional>
#include <vector>

struct QueueFamilyIndices
{
    std::optional<std::uint32_t> m_graphics_family {};
    std::optional<std::uint32_t> m_present_family  {};

    bool is_complete() const { return m_graphics_family.has_value() && m_present_family.has_value(); }
};

class Vulkan
{
public:
    Vulkan() = default;
    ~Vulkan();

    // Delete copy constructor and copy assignment
    Vulkan(const Vulkan&) = delete;
    Vulkan& operator=(const Vulkan&) = delete;

    // Delete move constructor and move assignment
    Vulkan(Vulkan&&) = delete;
    Vulkan& operator=(Vulkan&&) = delete;

    void start();

private:
    void initialize_window();

    void initialize_vulkan();
    void create_instance(bool show_extensions);
    void setup_debug_messenger();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();

    bool is_device_suitable(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();
    void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);  
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);

    void main_loop();

    void cleanup();

    GLFWwindow*              m_window          {};
    VkInstance               m_instance        {};
    VkDebugUtilsMessengerEXT m_debug_messenger {};
    VkPhysicalDevice         m_physical_device {};
    VkDevice                 m_device          {};
    VkQueue                  m_graphics_queue  {};
    VkSurfaceKHR             m_surface         {};
    VkQueue                  m_present_queue   {};
};