#pragma once

#include <GLFW/glfw3.h>
#include <string>


class Window {
public:
	Window() {}
	Window(int w, int h, std::string name) : width(w), height(h), windowName(name)
	{
		initWindow();
	}
	~Window() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	bool should_close();
	GLFWwindow * window;

private:
	void initWindow();
	uint32_t width = 800;
	uint32_t height = 600;
	std::string windowName;
};

void Window::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

bool Window::should_close() {
	return glfwWindowShouldClose(window);
}