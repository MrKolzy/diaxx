#include "diaxx/vulkan.h"

#include <iostream>

int main()
{
	Diaxx::Vulkan vulkan {};

	try
	{
		vulkan.start();
	}
	catch (const std::exception& exception)
	{
		std::print(std::cerr, "{}", exception.what());

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}