#pragma once

#include "Interactor.h"
#include "ApplicationVisualize.h"
#include "SetupGLFW.h"

#include <GLFW/glfw3.h>

class CVisualizer
{
public:
	void init()
	{
		_ASSERTE(!m_IsInitted);
		GLFW::init();
		m_pWindow = GLFW::createWindow(1280, 800, "Visualize");
		m_pApp = make<CApplicationVisualize>();
		m_pApp->create(m_pWindow);
		m_IsInitted = true;
	}

	void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
	{
		m_pApp->addTriangle(vTriangle, vColor);
	}

	void addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
	{
		m_pApp->addLine(vLine, vColor);
	}

	void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
	{
		m_pApp->addPoint(vPoint, vColor);
	}

	void start()
	{
		_ASSERTE(m_IsInitted);

		GLFW::startLoop(m_pWindow, [=]()
			{
				m_pApp->tick();
			},
			[=](int vWidth, int vHeight)
			{
				m_pApp->resize(vWidth, vHeight);
			});

		__destory();
	}
private:
	void __destory()
	{
		m_pApp->waitDevice();
		m_pApp->destroy();
		GLFW::destroyWindow(m_pWindow);
		GLFW::terminate();
	}

	bool m_IsInitted = false;
	GLFWwindow* m_pWindow = nullptr;
	ptr<CApplicationVisualize> m_pApp = nullptr;
};