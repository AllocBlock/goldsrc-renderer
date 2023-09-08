#pragma once
#include "ImageUtils.h"
#include "PassFullScreen.h"
#include "PipelineBloomLuminance.h"
#include "PipelineBloomBlur.h"
#include "PipelineBloomMerge.h"

// references:
// Unity Shader - Bloom(光晕、泛光): https://zhuanlan.zhihu.com/p/140724673
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

        VkFormat InputFormat = m_pPortSet->getInputPort("Main")->getActualFormatV();
        VkFormat MergeFormat = m_pPortSet->getOutputPort("Main")->getActualFormatV();
        m_LuminanceImageSet.init(m_ImageNum);
        m_BlurredImageSet.init(m_ImageNum);
        m_OutputImageSet.init(m_ImageNum);
        for (size_t i = 0; i < m_ImageNum; ++i)
        {
            ImageUtils::createImage2d(*m_LuminanceImageSet[i], m_pDevice, RefExtent, InputFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            ImageUtils::createImage2d(*m_BlurredImageSet[i], m_pDevice, RefExtent, InputFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            ImageUtils::createImage2d(*m_OutputImageSet[i], m_pDevice, RefExtent, MergeFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            m_pPortSet->setOutput("Main", *m_OutputImageSet[i], i);
        }

        CRenderPassFullScreen::_initV();

        m_LuminiancePipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 0);
        m_BlurPipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 1);
        m_MergePipeline.create(m_pDevice, get(), RefExtent, m_ImageNum, 2);
        for (size_t i = 0; i < m_ImageNum; ++i)
        {
            VkImageView InputImage = m_pPortSet->getInputPort("Main")->getImageV(i);
            m_LuminiancePipeline.setInputImage(InputImage, i);
            m_BlurPipeline.setInputImage(*m_LuminanceImageSet[i], i);
            m_MergePipeline.setInputImage(InputImage, *m_BlurredImageSet[i], i);
        }
    }

    virtual void _initPortDescV(SPortDescriptor& vioDesc) override
    {
        vioDesc.addInput("Main", SPortFormat::createAnyOfUsage(EUsage::READ));
        vioDesc.addOutput("Main", SPortFormat{ VK_FORMAT_B8G8R8A8_UNORM, SPortFormat::AnyExtent, 0, EUsage::WRITE });
    }

    void _destroyV() override
    {
        m_LuminanceImageSet.destroyAndClearAll();
        m_BlurredImageSet.destroyAndClearAll();
        m_OutputImageSet.destroyAndClearAll();
        m_LuminiancePipeline.destroy();
        m_BlurPipeline.destroy();
        m_MergePipeline.destroy();
        CRenderPassFullScreen::_destroyV();
    }

    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        CRenderPassDescriptor Desc;

        Desc.addColorAttachment(m_pPortSet->getOutputPort("Main"));

        VkFormat InputFormat = m_pPortSet->getInputPort("Main")->getActualFormatV();

        SAttachementInfo LuminanceAttachementInfo;
        LuminanceAttachementInfo.Format = InputFormat;
        LuminanceAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        LuminanceAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        LuminanceAttachementInfo.IsBegin = true;
        LuminanceAttachementInfo.IsEnd = false;
        Desc.addColorAttachment(LuminanceAttachementInfo);

        SAttachementInfo BlurredAttachementInfo;
        BlurredAttachementInfo.Format = InputFormat;
        BlurredAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        BlurredAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        BlurredAttachementInfo.IsBegin = true;
        BlurredAttachementInfo.IsEnd = false;
        Desc.addColorAttachment(BlurredAttachementInfo);


        Desc.addSubpass(SSubpassReferenceInfo()
            .addColorRef(1)
            .addDependentPass(VK_SUBPASS_EXTERNAL)
        ); // luminance
        Desc.addSubpass(SSubpassReferenceInfo()
            .addColorRef(2)
            .addInputRef(1)
            .addDependentPass(0)
        ); // blur
        Desc.addSubpass(SSubpassReferenceInfo()
            .addColorRef(0)
            .addInputRef(2)
            .addDependentPass(1)
        ); // merge
        return Desc;
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

        _beginWithFramebuffer(vImageIndex);

        m_LuminiancePipeline.bind(pCommandBuffer, vImageIndex);
        _drawFullScreen(pCommandBuffer);
        pCommandBuffer->goNextPass();
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
            *m_LuminanceImageSet[vIndex],
            *m_BlurredImageSet[vIndex]
        };
    }

    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        std::vector<VkClearValue> ValueSet(3);
        ValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ValueSet[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ValueSet[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        return ValueSet;
    }

    virtual void _renderUIV() override
    {
        if (UI::collapse(u8"Bloom"))
        {
            //UI::drag(u8"模糊迭代次数", m_FilterIteration, 1, 1, 5); // TODO: implement multiple iter blur
            UI::drag(u8"模糊范围", m_FilterSize, 2, 3, 15);
            UI::drag(u8"强度", m_BloomFactor, 0.05f, 0.0f, 5.0f);
        }
    }

    virtual void _updateV(uint32_t vImageIndex) override
    {
        m_BlurPipeline.updateUniformBuffer(vImageIndex, uint32_t(m_FilterSize));
        m_MergePipeline.updateUniformBuffer(vImageIndex, m_BloomFactor);
    }

private:

    vk::CPointerSet<vk::CImage> m_LuminanceImageSet;
    vk::CPointerSet<vk::CImage> m_BlurredImageSet;
    vk::CPointerSet<vk::CImage> m_OutputImageSet;

    CPipelineBloomLuminance m_LuminiancePipeline;
    CPipelineBloomBlur m_BlurPipeline;
    CPipelineBloomMerge m_MergePipeline;

    float m_BloomFactor = 0.5f;
    int m_FilterIteration = 3;
    int m_FilterSize = 5;
};
