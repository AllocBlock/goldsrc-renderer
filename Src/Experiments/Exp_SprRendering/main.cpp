#include "ApplicationSprite.h"
#include "SetupGLFW.h"

int main()
{
	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "Spr Rendering Test");
	ptr<CApplicationSprite> pApp = make<CApplicationSprite>();
	pApp->create(pWindow);

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