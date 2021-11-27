#pragma once

#include <GLFW/glfw3.h>
#include <functional>

namespace GLFW
{
	void init();
	GLFWwindow* createWindow(int vWidth, int vHeight, std::string vTitle = "≤‚ ‘");
	void startLoop(GLFWwindow* vWindow, std::function<void()> vLoopCallback);
	void destroyWindow(GLFWwindow* vWindow);
	void terminate();
}
