#include "diaxx/vulkan.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
	diaxx::Vulkan vulkan {};

	try
	{
		vulkan.run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what();

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}