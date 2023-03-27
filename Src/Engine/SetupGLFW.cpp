#include "SetupGLFW.h"

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

// FIXME: multi thread unsafe
int LastWidth = 0;
int LastHeight = 0;

void GLFW::startLoop(GLFWwindow* vWindow, std::function<void()> vLoopCallback, std::function<void(int, int)> vResizeCallback)
{
	glfwGetFramebufferSize(vWindow, &LastWidth, &LastHeight);

	while (!glfwWindowShouldClose(vWindow))
	{
		glfwPollEvents();
		int Width = 0, Height = 0;
		glfwGetFramebufferSize(vWindow, &Width, &Height);
		if (vResizeCallback && (LastWidth != Width || LastHeight != Height))
		{
			LastWidth = Width;
			LastHeight = Height;
			vResizeCallback(Width, Height);
		}
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