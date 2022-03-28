#include "ApplicationTest.h"
#include "SetupGLFW.h"

int main()
{
	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "PBR Test");
	ptr<CApplicationTest> pApp = make<CApplicationTest>();
	pApp->init(pWindow);

	GLFW::startLoop(pWindow, [=]()
	{
		pApp->render();
	});

	pApp->waitDevice();
	pApp->destroy();
	GLFW::destroyWindow(pWindow);
	GLFW::terminate();
	return 0;
}