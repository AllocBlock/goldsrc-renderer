#include "Interactor.h"
#include "ImguiVullkan.h"

#include <GLFW/glfw3.h>

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	std::shared_ptr<CImguiVullkan> pImgui = std::make_shared<CImguiVullkan>(pWindow);
	pImgui->init();

	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		pImgui->render();
	}
	pImgui->waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();

	pImgui->destroy();
	return 0;
}