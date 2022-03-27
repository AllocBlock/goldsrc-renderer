#pragma once
#include "RendererBase.h"
#include "FrameBuffer.h"
#include "PipelineTest.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "MaterialPBR.h"

class CRendererTest : public CRendererBase
{
public:
    CRendererTest() : m_pCamera(std::make_shared<CCamera>()) {}

    std::shared_ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
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
    void __createMaterials();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, glm::vec3 vCenter, uint32_t vMaterialIndex, int vDepth);
   
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CPipelineTest m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::vector<std::shared_ptr<vk::CFrameBuffer>> m_FramebufferSet;
    std::shared_ptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    std::shared_ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    std::shared_ptr<vk::CImage> m_pDepthImage = nullptr;

    const std::string m_SkyFilePrefix = "../../data/neb6";
    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::vector<STestPointData> m_PointDataSet;
    std::array<std::shared_ptr<CIOImage>, 6> m_SkyBoxImageSet;

    uint32_t m_GridSize = 8;

    bool m_ForceUseMat = false;
    bool m_UseNormalMap = false;
    SMaterialPBR m_Material;
    std::vector<vk::CImage::Ptr> m_TextureColorSet;
    std::vector<vk::CImage::Ptr> m_TextureNormalSet;
    std::vector<vk::CImage::Ptr> m_TextureSpecularSet;
};

