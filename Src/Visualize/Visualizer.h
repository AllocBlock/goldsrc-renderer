#pragma once
#include "Pointer.h"
#include "VisualizePrimitive.h"
#include "Interactor.h"
#include "ApplicationVisualize.h"
#include "SetupGLFW.h"

class CVisualizer
{
public:
	void init(int vWidth = 1280, int vHeight = 800, std::string vTitle = "Visualize");
    void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor);
    void addLine(const Visualize::Line& vLine, const glm::vec3& vColor);
    void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor);
    void addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);
    void addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);
    void start();

private:
	void __destroy();
    
    bool m_IsInitted = false;
	GLFWwindow* m_pWindow = nullptr;
	ptr<CApplicationVisualize> m_pApp = nullptr;
};