#pragma once
#include "..\RenderPassSingleFrameBuffer.h"
#include "Camera.h"
#include "DynamicResourceManager.h"
#include "Image.h"
#include "MaterialPBR.h"
#include "PipelinePBS.h"
#include "VertexBuffer.h"

class CRenderPassPBR : public engine::IRenderPass
{
public:
    CRenderPassPBR() = default;

    void setCamera(sptr<CCamera> vCamera) { m_pCamera = vCamera; }
    sptr<CCamera> getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual sptr<CPortSet> _createPortSetV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Main", voExtent);
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
            m_DepthImageManager.getImageViewV()
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColorDepth;
    }

private:
    void __createVertexBuffer();
    void __createMaterials();

    void __updateUniformBuffer(uint32_t vImageIndex);
    void __generateScene();
    void __subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, glm::vec3 vCenter, uint32_t vMaterialIndex, int vDepth);
   
    CDynamicPipeline<CPipelinePBS> m_PipelineCreator;
    sptr<vk::CVertexBuffer> m_pVertexBuffer = nullptr;
    sptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    CDynamicTextureCreator m_DepthImageManager;

    sptr<CCamera> m_pCamera = nullptr;
    std::vector<CPipelinePBS::SPointData> m_PointDataSet;

    uint32_t m_GridSize = 8;
     
    CPipelinePBS::SControl m_PipelineControl; 
    vk::CPointerSet<vk::CImage> m_TextureColorSet;
    vk::CPointerSet<vk::CImage> m_TextureNormalSet;
    vk::CPointerSet<vk::CImage> m_TextureSpecularSet;
};

