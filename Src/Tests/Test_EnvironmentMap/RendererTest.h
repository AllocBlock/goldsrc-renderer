#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "PipelineTest.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"

class CRendererTest : public vk::IRenderPass
{
public:
    CRendererTest() : m_pCamera(make<CCamera>()) {}

    ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __loadSkyBox();
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension);
    void __createRenderPass();
    void __createGraphicsPipeline();
    void __createDepthResources();
    void __createFramebuffers();
    void __createVertexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth);
   
    CPipelineTest m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    vk::CImage m_DepthImage = nullptr;

    const std::string m_SkyFilePrefix = "../../../ExampleData/sky-test/neb6";
    ptr<CCamera> m_pCamera = nullptr;
    std::vector<STestPointData> m_PointDataSet;
    std::array<ptr<CIOImage>, 6> m_SkyBoxImageSet;
};

