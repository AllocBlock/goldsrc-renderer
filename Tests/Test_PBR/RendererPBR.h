#pragma once
#include "Renderer.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "MaterialPBR.h"
#include "PipelinePBS.h"

class CRendererPBR : public IRenderer
{
public:
    CRendererPBR() : m_pCamera(make<CCamera>()) {}

    ptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
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
    CPipelinePBS m_Pipeline;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Test";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    vk::CImage::Ptr m_pDepthImage = nullptr;

    const std::string m_SkyFilePrefix = "../../data/neb6";
    ptr<CCamera> m_pCamera = nullptr;
    std::vector<SPBSPointData> m_PointDataSet;
    std::array<ptr<CIOImage>, 6> m_SkyBoxImageSet;

    uint32_t m_GridSize = 8;

    CPipelinePBS::SControl m_PipelineControl;
    std::vector<vk::CImage::Ptr> m_TextureColorSet;
    std::vector<vk::CImage::Ptr> m_TextureNormalSet;
    std::vector<vk::CImage::Ptr> m_TextureSpecularSet;
};

