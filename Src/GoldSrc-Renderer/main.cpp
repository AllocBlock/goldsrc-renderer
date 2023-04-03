#include "Interactor.h"
#include "ApplicationGoldSrc.h"
#include "SetupGLFW.h"

#include <GLFW/glfw3.h>

int main()
{
	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "Vulkan Simple Render");
	ptr<CApplicationGoldSrc> pApp = make<CApplicationGoldSrc>();
	pApp->create(pWindow);
	 
	GLFW::startLoop(pWindow, [=]() 
	{
		pApp->tick();
	},
		[=](int vWidth, int vHeight)
	{
		pApp->resize(vWidth, vHeight);
	});

	pApp->waitDevice();
	pApp->destroy();
	GLFW::destroyWindow(pWindow);
	GLFW::terminate();
	return 0;
}
