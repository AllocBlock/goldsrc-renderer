#pragma once
#include "RenderPass.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "PipelineTest.h"
#include "RenderInfoDescriptor.h"

class CRenderPassSprite : public engine::IRenderPass
{
public:
    CRenderPassSprite() : m_pCamera(make<CCamera>()) {}

    sptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual sptr<CPortSet> _createPortSetV() override;
    virtual void _updateV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;

private:
    void __loadSkyBox();
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension);
    void __createVertexBuffer();

    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth);

    CRenderInfoDescriptor m_RenderInfoDescriptor;
    CPipelineTest m_Pipeline;
    sptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    sptr<vk::CImage> m_pMainImage;
    sptr<vk::CImage> m_pDepthImage;

    const std::string m_SkyFilePrefix = "../../../ExampleData/sky-test/neb6";
    sptr<CCamera> m_pCamera = nullptr;
    std::vector<CPipelineTest::SPointData> m_PointDataSet;
    std::array<sptr<CIOImage>, 6> m_SkyBoxImageSet;
};

