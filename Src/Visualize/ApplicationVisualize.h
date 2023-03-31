#pragma once
#include "Application.h"
#include "VisualizePrimitive.h"
#include "Interactor.h"
#include "PassVisualize.h"

class CApplicationVisualize : public IApplication
{
public:
    CApplicationVisualize() = default;
    void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor);
    void addLine(const Visualize::Line& vLine, const glm::vec3& vColor);
    void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor);
    void addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onResizeV() override;

private:
    void __linkPasses();

    CDynamicTextureCreator m_DepthImageManager;
    CSourcePort::Ptr m_pDepthPort = nullptr;
    ptr<CRenderPassVisualize> m_pPassVisualize = nullptr;

    ptr<CInteractor> m_pInteractor = nullptr;
    CCamera::Ptr m_pCamera = nullptr;
};
