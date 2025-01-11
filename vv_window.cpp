#include "vv_window.h"
#include <stdexcept>

namespace vv {
	VvWindow::VvWindow(int w, int h, std::string name) : width(w), height(h), windowName(name) {
		initWindow();
	}

	VvWindow::~VvWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VvWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface");
		}
	}

	void VvWindow::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto vvWindow = reinterpret_cast<VvWindow*>(glfwGetWindowUserPointer(window));

		vvWindow->frameBufferResized = true;
		vvWindow->width = width;
		vvWindow->height = height;
	}

	void VvWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
	}
}