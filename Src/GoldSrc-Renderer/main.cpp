#include "Interactor.h"
#include "ApplicationGoldSrc.h"
#include "SetupGLFW.h"
#include "RenderPassRegister.h"

#include <GLFW/glfw3.h>

#define _REGISTER_PASS(Name) CRenderPassRegister<CRenderPass##Name> Register##Name(#Name)

// TODO: remove this and use self-register
void registerAllPasses()
{
	_REGISTER_PASS(GoldSrc);
	_REGISTER_PASS(OutlineEdge);
	_REGISTER_PASS(OutlineMask);
	_REGISTER_PASS(Visualize);
	_REGISTER_PASS(GUI);
}

int main()
{
	// TODO: remove this and use self-register
	registerAllPasses();

	GLFW::init();
	GLFWwindow* pWindow = GLFW::createWindow(1280, 800, "Vulkan Render");
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
