#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Image.h"
#include "Buffer.h"
#include "MaterialPBR.h"
#include "PipelinePBS.h"

class CRenderPassPBR : public vk::IRenderPass
{
public:
    CRenderPassPBR() = default;

    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; }
    CCamera::Ptr getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

private:
    void __createGraphicsPipeline();
    void __createDepthResources();
    void __createFramebuffers();
    void __createVertexBuffer();
    void __createMaterials();

    void __createRecreateResources();
    void __destroyRecreateResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, glm::vec3 vCenter, uint32_t vMaterialIndex, int vDepth);
   
    CPipelinePBS m_Pipeline;
    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;
    ptr<vk::CBuffer> m_pVertexBuffer = nullptr;
    ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    vk::CImage m_DepthImage; 

    CCamera::Ptr m_pCamera = nullptr;
    std::vector<CPipelinePBS::SPointData> m_PointDataSet;

    uint32_t m_GridSize = 8;
     
    CPipelinePBS::SControl m_PipelineControl; 
    vk::CPointerSet<vk::CImage> m_TextureColorSet;
    vk::CPointerSet<vk::CImage> m_TextureNormalSet;
    vk::CPointerSet<vk::CImage> m_TextureSpecularSet;
};

