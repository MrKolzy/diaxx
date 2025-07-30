#pragma once

#define  GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <vector>

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
};