#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "PipelineTest.h"

class CRenderPassSprite : public vk::IRenderPass
{
public:
    CRenderPassSprite() : m_pCamera(make<CCamera>()) {}

    CCamera::Ptr getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __loadSkyBox();
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension);
    void __createGraphicsPipeline();
    void __createDepthResources();
    void __createFramebuffers(VkExtent2D vExtent);
    void __createVertexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth);
   
    CPipelineTest m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    vk::CImage m_DepthImage;

    const std::string m_SkyFilePrefix = "../../../ExampleData/sky-test/neb6";
    CCamera::Ptr m_pCamera = nullptr;
    std::vector<CPipelineTest::SPointData> m_PointDataSet;
    std::array<ptr<CIOImage>, 6> m_SkyBoxImageSet;
};

