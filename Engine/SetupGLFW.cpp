#include "SetupGLFW.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void GLFW::init()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GLFWwindow* GLFW::createWindow(int vWidth, int vHeight, std::string vTitle)
{
	GLFWwindow* pWindow = glfwCreateWindow(vWidth, vHeight, vTitle.c_str(), NULL, NULL);
	if (!pWindow)
	{
		terminate();
		throw "GLFW´´½¨´°¿ÚÊ§°Ü";
	}
	glfwMakeContextCurrent(pWindow);
	return pWindow;
}

void GLFW::startLoop(GLFWwindow* vWindow, std::function<void()> vLoopCallback)
{
	while (!glfwWindowShouldClose(vWindow))
	{
		glfwPollEvents();
		vLoopCallback();
	}
}

void GLFW::destroyWindow(GLFWwindow* vWindow)
{
	glfwDestroyWindow(vWindow);
}
void GLFW::terminate()
{
	glfwTerminate();
}