#include "Interactor.h"
#include "ApplicationGoldSrc.h"
#include "SetupGLFW.h"
#include "RenderpassLib.h"

#include "PassGoldSrc.h"
#include "PassGUI.h"
#include "PassOutlineMask.h"
#include "PassOutlineEdge.h"
#include "PassVisualize.h"

#include <GLFW/glfw3.h>

#define _REGISTER_PASS(Type) CRenderPassRegister<Type> Register##Type

// TODO: remove this and use self-register
void registerAllPasses()
{
	_REGISTER_PASS(CRenderPassGoldSrc);
	_REGISTER_PASS(CRenderPassOutlineEdge);
	_REGISTER_PASS(CRenderPassOutlineMask);
	_REGISTER_PASS(CRenderPassVisualize);
	_REGISTER_PASS(CRenderPassGUI);
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
