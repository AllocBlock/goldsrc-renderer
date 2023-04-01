#include "Visualizer.h"

void CVisualizer::init(int vWidth, int vHeight, std::string vTitle)
{
    _ASSERTE(!m_IsInitted);
    GLFW::init();
    m_pWindow = GLFW::createWindow(vWidth, vHeight, vTitle);
    m_pApp = make<CApplicationVisualize>();
    m_pApp->create(m_pWindow);
    m_IsInitted = true;
}

void CVisualizer::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{
    _ASSERTE(m_IsInitted);
    m_pApp->addTriangle(vTriangle, vColor);
}

void CVisualizer::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{
    _ASSERTE(m_IsInitted);
    m_pApp->addLine(vLine, vColor);
}

void CVisualizer::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{
    _ASSERTE(m_IsInitted);
    m_pApp->addPoint(vPoint, vColor);
}

void CVisualizer::addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_pApp->addSphere(vCenter, vScale, vColor);
}

void CVisualizer::addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_pApp->addCube(vCenter, vScale, vColor);
}

void CVisualizer::start()
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

    __destroy();
}

void CVisualizer::__destroy()
{
    m_pApp->waitDevice();
    m_pApp->destroy();
    GLFW::destroyWindow(m_pWindow);
    GLFW::terminate();
}
