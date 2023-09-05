#pragma once
#include "DynamicResourceManager.h"
#include "PassFullScreen.h"
#include "PipelineBloomBlur.h"
#include "PipelineBloomMerge.h"
#include "RerecordState.h"

// references:
// Unity Shader - Bloom(π‚‘Œ°¢∑∫π‚): https://zhuanlan.zhihu.com/p/140724673
class CRenderPassBlend : public CRenderPassFullScreenGeneral
{
protected:

    virtual void _initV() override
    {
        CRenderPassFullScreenGeneral::_initV();

        m_pRerecord = make<CRerecordState>(m_pAppInfo);
        m_pRerecord->addField("Primary");

        VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();

        m_BlurImageCreator.init(ScreenExtent, true,
            [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
            {
                VkFormat Format = VkFormat::VK_FORMAT_R8G8B8_SNORM;

                vImageSet.init(m_pAppInfo->getImageNum());
                for (size_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
                {
                    //ImageUtils::createImage2d(vImageSet[i], m_pDevice, vExtent, Format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
                    ImageUtils::createImage2d(*vImageSet[i], m_pDevice, vExtent, Format,  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
                }
            }
        );

        auto pRerecordCallback = [this](IPipeline& vPipeline) { m_pRerecord->requestRecordForAll(); };
        
        m_BlurPipelineCreator.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), pRerecordCallback, 0
        );

        m_MergePipelineCreator.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), pRerecordCallback, 1);
        
        m_pRerecord->requestRecordForAll();
    }

    virtual void _initPortDescV(SPortDescriptor& vioDesc) override
    {
        CRenderPassDescriptor Desc;

        Desc.addColorAttachment(m_pPortSet->getOutputPort("Main"));

        SAttachementInfo BlurredAttachementInfo;
        BlurredAttachementInfo.Format = VkFormat::VK_FORMAT_R8G8B8_SNORM;
        BlurredAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        BlurredAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        BlurredAttachementInfo.IsBegin = true;
        BlurredAttachementInfo.IsEnd = false;
        Desc.addColorAttachment(BlurredAttachementInfo);
        
        Desc.addSubpass({ 1 }, false, { VK_SUBPASS_EXTERNAL }, {}); // blur
        Desc.addSubpass({ 0 }, false, { 0 }, { 1 }); // merge
        return Desc;
    }

    void _destroyV() override
    {
        m_BlurImageCreator.destroy();
        m_BlurPipelineCreator.destroy();
        m_MergePipelineCreator.destroy();
        CRenderPassFullScreenGeneral::_destroyV();
    }
    
    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getInputPort("Input"));
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
        if (m_pRerecord->consume("Primary"))
        {
            _beginWithFramebuffer(vImageIndex);

            m_BlurPipelineCreator.get().bind(pCommandBuffer, vImageIndex);
            _drawFullScreen(pCommandBuffer);
            pCommandBuffer->goNextPass();
            m_MergePipelineCreator.get().bind(pCommandBuffer, vImageIndex);
            _drawFullScreen(pCommandBuffer);

            _endWithFramebuffer();
        }
        return { pCommandBuffer->get() };
    }
    
    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override
    {
        m_BlurImageCreator.updateV(vUpdateState);
        m_BlurPipelineCreator.updateV(vUpdateState);
        m_MergePipelineCreator.updateV(vUpdateState);

        CRenderPassFullScreenGeneral::_onUpdateV(vUpdateState);
    }

    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Output")->getImageV(vIndex),
            m_BlurImageCreator.getImageViewV(vIndex)
        };
    }

private:

    vk::CPointerSet<vk::CImage> m_BrightImageSet;

    CDynamicTextureCreator m_BlurImageCreator;
    CDynamicPipeline<CPipelineBloomBlur> m_BlurPipelineCreator;
    CDynamicPipeline<CPipelineBloomMerge> m_MergePipelineCreator;

    CRerecordState::Ptr m_pRerecord = nullptr;
};
