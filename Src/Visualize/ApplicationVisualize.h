#pragma once
#include "Application.h"
#include "VisualizePrimitive.h"
#include "Interactor.h"
#include "PassVisualize.h"

// TODO broken, fix later
class CApplicationVisualize : public IApplication
{
public:
    CApplicationVisualize() = default;
    void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor);
    void addLine(const Visualize::Line& vLine, const glm::vec3& vColor);
    void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor);
    void addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);
    void addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t ImageIndex) override;
    virtual std::vector<VkCommandBuffer> _getCommandBuffers() override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    ptr<CRenderPassVisualize> m_pPassVisualize = nullptr;
    ptr<CRenderPassPresent> m_pPassPresent = nullptr;

    ptr<CInteractor> m_pInteractor = nullptr;
    ptr<SSceneInfo> m_pSceneInfo = make<SSceneInfo>();
};
