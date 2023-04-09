#pragma once
#include "RenderPassSingle.h"
#include "Camera.h"
#include "DynamicResourceManager.h"
#include "Image.h"
#include "MaterialPBR.h"
#include "PipelinePBS.h"
#include "VertexBuffer.h"
#include "UniformBuffer.h"

class CRenderPassPBR : public CRenderPassSingle
{
public:
    CRenderPassPBR() = default;

    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; }
    CCamera::Ptr getCamera() { return m_pCamera; }

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
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
    vk::CVertexBuffer::Ptr m_pVertexBuffer = nullptr;
    vk::CBuffer::Ptr m_pMaterialBuffer = nullptr;
    CDynamicTextureCreator m_DepthImageManager;

    CCamera::Ptr m_pCamera = nullptr;
    std::vector<CPipelinePBS::SPointData> m_PointDataSet;

    uint32_t m_GridSize = 8;
     
    CPipelinePBS::SControl m_PipelineControl; 
    vk::CPointerSet<vk::CImage> m_TextureColorSet;
    vk::CPointerSet<vk::CImage> m_TextureNormalSet;
    vk::CPointerSet<vk::CImage> m_TextureSpecularSet;
};

