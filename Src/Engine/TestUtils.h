#include "Pointer.h"
#include "SetupGLFW.h"

template <typename App_t>
int runTestApp(std::string vTitle, int vWindowWidth = 1280, int vWindowHeight = 800)
{
	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "Spr Rendering Test");
	ptr<App_t> pApp = make<App_t>();
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
