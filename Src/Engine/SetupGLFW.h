#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>
#include <string>

namespace GLFW
{
	void init();
	GLFWwindow* createWindow(int vWidth, int vHeight, std::string vTitle = "≤‚ ‘");
	void startLoop(GLFWwindow* vWindow, std::function<void()> vLoopCallback, std::function<void(int, int)> vResizeCallback = nullptr);
	void destroyWindow(GLFWwindow* vWindow);
	void terminate();
}
