#include "Interactor.h"
#include "ApplicationGoldSrc.h"

#include <GLFW/glfw3.h>

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(1280, 800, "Vulkan Simple Render", nullptr, nullptr);
	std::shared_ptr<CApplicationGoldSrc> pApp = std::make_shared<CApplicationGoldSrc>();
	pApp->init(pWindow);

	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		pApp->render();
	}
	pApp->waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();

	pApp->destroy();
	return 0;
}