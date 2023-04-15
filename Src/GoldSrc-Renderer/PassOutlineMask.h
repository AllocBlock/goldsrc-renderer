#pragma once
#include "RenderPassSingleFrameBuffer.h"
#include "SceneInfo.h"
#include "PipelineOutlineMask.h"
#include "DynamicResourceManager.h"

class CRenderPassOutlineMask : public CRenderPassSingleFrameBuffer
{
public:
    inline static const std::string Name = "OutlineMask";
    CRenderPassOutlineMask() = default;

    void setHighlightActor(CActor::Ptr vActor);
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

    CDynamicPipeline<CPipelineMask> m_PipelineCreator;
    CDynamicTextureCreator m_MaskImageCreator;

    size_t m_RerecordCommandTimes = 0;
};
