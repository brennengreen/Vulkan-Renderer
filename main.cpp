#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "application.h"

#define WIDTH 800
#define HEIGHT 600

int main() {
	Application app(WIDTH, HEIGHT);

	try 
	{
		app.run();
	}
	catch (const std::exception & e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}