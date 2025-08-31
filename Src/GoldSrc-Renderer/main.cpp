#include "Interactor.h"
#include "ApplicationGoldSrc.h"
#include "SetupGLFW.h"
#include "RenderpassLib.h"

#include "PassGoldSrc.h"
#include "PassGUI.h"
#include "PassOutline.h"
#include "PassVisualize.h"
#include "PassBloom.h"
#include "PassSSAO.h"
#include "PassPresent.h"

#include <GLFW/glfw3.h>

#define _REGISTER_PASS(Type, Visible) CRenderPassRegister<Type, Visible> Register##Type

// TODO: remove this and use self-register
void registerAllPasses()
{
	_REGISTER_PASS(CRenderPassGoldSrc, true);
	//_REGISTER_PASS(CRenderPassOutline, true);
	//_REGISTER_PASS(CRenderPassVisualize, true);
	//_REGISTER_PASS(CRenderPassBloom, true);
	_REGISTER_PASS(CRenderPassSSAO, true);
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
