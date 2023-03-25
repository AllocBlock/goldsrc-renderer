#pragma once
#include "RenderPassSingle.h"
#include "Camera.h"
#include "Image.h"
#include "SceneInfoGoldSrc.h"
#include "PipelineOutlineMask.h"
#include "DynamicResourceManager.h"

class CRenderPassOutlineMask : public CRenderPassSingle
{
public:
    CRenderPassOutlineMask() = default;

    CCamera::Ptr getCamera() { return m_pCamera; }
    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; }

    void setHighlightActor(CActor<CMeshDataGoldSrc>::Ptr vActor);
    void removeHighlight();

protected:
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        voExtent = m_pAppInfo->getScreenExtent();
        return true;
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Mask")->getImageV(vIndex)
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColor;
    }

private:
    void __rerecordCommand();

    CCamera::Ptr m_pCamera = nullptr;

    CPipelineMask m_PipelineMask;

    CDynamicTextureCreator m_MaskImageCreator;

    size_t m_RerecordCommandTimes = 0;
};
