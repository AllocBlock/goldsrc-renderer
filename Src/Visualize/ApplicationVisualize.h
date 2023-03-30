#pragma once
#include "Application.h"
#include "VisualizePrimitive.h"
#include "Interactor.h"
#include "PassVisualize.h"

class CApplicationVisualize : public IApplication
{
public:
    CApplicationVisualize() = default;
    void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor) { m_pPassVisualize->addTriangle(vTriangle, vColor); }
    void addLine(const Visualize::Line& vLine, const glm::vec3& vColor) { m_pPassVisualize->addLine(vLine, vColor); }
    void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor) { m_pPassVisualize->addPoint(vPoint, vColor); }

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    ptr<CRenderPassVisualize> m_pPassVisualize = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    CCamera::Ptr m_pCamera = nullptr;
};
