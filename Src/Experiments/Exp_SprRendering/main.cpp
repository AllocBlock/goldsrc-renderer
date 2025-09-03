#include "ApplicationSprite.h"
#include "SetupGLFW.h"

int main()
{
	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "Spr Rendering Test");
	sptr<CApplicationSprite> pApp = make<CApplicationSprite>();
	pApp->create(pWindow);

	GLFW::startLoop(pWindow, [=]()
	{
		pApp->tick();
	}, [=](int vWidth, int vHeight)
	{
		pApp->resize(vWidth, vHeight);
	});

	pApp->waitDevice();
	pApp->destroy();
	GLFW::destroyWindow(pWindow);
	GLFW::terminate();
	return 0;
}