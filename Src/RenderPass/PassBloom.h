#pragma once
#include "PassFullScreen.h"
#include "PipelineBloomBlur.h"
#include "PipelineBloomMerge.h"

// references:
// Unity Shader - Bloom(¹âÔÎ¡¢·º¹â): https://zhuanlan.zhihu.com/p/140724673
class CRenderPassBloom : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "Bloom";

protected:
    virtual void _initV() override
    {
        VkExtent2D RefExtent = vk::ZeroExtent;
        bool Success = _dumpReferenceExtentV(RefExtent);
        _ASSERTE(Success);

        VkFormat Format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
        ImageUtils::createImage2d(m_BlurImage, m_pDevice, RefExtent, Format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

        CRenderPassFullScreen::_initV();
        
        m_BlurPipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 0);
        m_MergePipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 1);
    }

    virtual void _initPortDescV(SPortDescriptor& vioDesc) override
    {
        vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    }

    void _destroyV() override
    {
        m_BlurImage.destroy();
        m_BlurPipeline.destroy();
        m_MergePipeline.destroy();
        CRenderPassFullScreen::_destroyV();
    }
    
    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        CRenderPassDescriptor Desc;

        Desc.addColorAttachment(m_pPortSet->getOutputPort("Main"));

        SAttachementInfo BlurredAttachementInfo;
        BlurredAttachementInfo.Format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
        BlurredAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        BlurredAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        BlurredAttachementInfo.IsBegin = true;
        BlurredAttachementInfo.IsEnd = false;
        Desc.addColorAttachment(BlurredAttachementInfo);

        Desc.addSubpass({ 1 }, false, { VK_SUBPASS_EXTERNAL }, {}); // blur
        Desc.addSubpass({ 0 }, false, { 0 }, { 1 }); // merge
        return Desc;
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
  
        _beginWithFramebuffer(vImageIndex);

        m_BlurPipeline.bind(pCommandBuffer, vImageIndex);
        _drawFullScreen(pCommandBuffer);
        pCommandBuffer->goNextPass();
        m_MergePipeline.bind(pCommandBuffer, vImageIndex);
        _drawFullScreen(pCommandBuffer);

        _endWithFramebuffer();
        return { pCommandBuffer->get() };
    }

    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
            m_BlurImage
        };
    }

    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        std::vector<VkClearValue> ValueSet(2);
        ValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ValueSet[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        return ValueSet;
    }

private:

    vk::CPointerSet<vk::CImage> m_BrightImageSet;

    vk::CImage m_BlurImage; // TODO: multiple image to fit swapchain
    CPipelineBloomBlur m_BlurPipeline;
    CPipelineBloomMerge m_MergePipeline;
};
