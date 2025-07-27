#pragma once

#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Vulkan
{
public:
    Vulkan();

private:
    void start();

    void initialize_window();
    void initialize_vulkan();
    void main_loop();
    void cleanup();

    GLFWwindow* m_window {};
};