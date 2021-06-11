#include "Interactor.h"
#include "GUIMain.h"

#include <GLFW/glfw3.h>

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(1280, 800, "Vulkan Simple Render", nullptr, nullptr);
	std::shared_ptr<CGUIMain> pImgui = std::make_shared<CGUIMain>(pWindow);
	pImgui->init(true);

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