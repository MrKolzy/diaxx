#include "diaxx/vulkan.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        Vulkan vulkan {};
        vulkan.start();
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}