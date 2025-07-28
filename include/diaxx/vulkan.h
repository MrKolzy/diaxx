#pragma once

#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

    void main_loop();

    void cleanup();

    GLFWwindow* m_window   {};
    VkInstance  m_instance {};
};