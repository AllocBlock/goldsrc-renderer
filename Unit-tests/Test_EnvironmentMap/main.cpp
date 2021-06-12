#include "ApplicationTest.h"

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(1280, 800, "Environment Mapping Test", nullptr, nullptr);
	std::shared_ptr<CApplicationTest> pApp = std::make_shared<CApplicationTest>();
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