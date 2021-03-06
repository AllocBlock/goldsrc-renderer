#pragma once
#include "Renderer.h"
#include "PipelineTest.h"
#include "Camera.h"

class CRendererTest : public CRenderer
{
public:
    CRendererTest() : m_pCamera(std::make_shared<CCamera>()) {}

    std::shared_ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __loadSkyBox();
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension);
    void __createRenderPass();
    void __destroyRenderPass();
    void __createGraphicsPipeline();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();
    void __createVertexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth);
   
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CPipelineTest m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::vector<VkFramebuffer> m_FramebufferSet;
    Common::SBufferPack m_VertexBufferPack;
    Common::SImagePack m_DepthImagePack;

    const std::string m_SkyFilePrefix = "../../data/neb6";
    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::vector<STestPointData> m_PointDataSet;
    std::array<std::shared_ptr<CIOImage>, 6> m_SkyBoxImageSet;
};

