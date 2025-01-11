#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace vv {
	class VvWindow {
	public:
		VvWindow(int w, int h, std::string name);
		~VvWindow();

		bool shouldClose() { 
			return glfwWindowShouldClose(window); 
		}

		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

		VvWindow(const VvWindow&) = delete;
		VvWindow& operator=(const VvWindow&) = delete;
		
		bool wasWindowResized() { return frameBufferResized; };
		void resetWindowResizedFlag() { frameBufferResized = false; };

		GLFWwindow* getGLFWwindow() const { return window; };

	private:
		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool frameBufferResized = false;

		std::string windowName;
		GLFWwindow* window;
	};
}