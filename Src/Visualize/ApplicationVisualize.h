#pragma once
#include "Application.h"
#include "VisualizePrimitive.h"
#include "Interactor.h"
#include "PassVisualize.h"

class CApplicationVisualize : public IApplication
{
public:
    CApplicationVisualize() = default;
    void addTriangle(const Visualize::Triangle& vTriangle) { m_pPassVisualize->addTriangle(vTriangle); }

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    ptr<CRenderPassVisualize> m_pPassVisualize = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    CCamera::Ptr m_pCamera = nullptr;

    std::vector<Visualize::Triangle> m_Triangles;
    std::vector<Visualize::Line> m_Lines;
    std::vector<Visualize::Point> m_Points;
};
