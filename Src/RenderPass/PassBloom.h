//#pragma once
//#include "ImageUtils.h"
//#include "PassFullScreen.h"
//#include "PipelineBloomLuminance.h"
//#include "PipelineBloomBlur.h"
//#include "PipelineBloomMerge.h"
//#include <InterfaceGui.h>
//
//// references:
//// Unity Shader - Bloom(光晕、泛光): https://zhuanlan.zhihu.com/p/140724673
//class CRenderPassBloom : public CRenderPassFullScreen
//{
//public:
//    inline static const std::string Name = "Bloom";
//
//protected:
//    virtual void _initV() override
//    {
//        VkExtent2D RefExtent = vk::ZeroExtent;
//        bool Success = _dumpReferenceExtentV(RefExtent);
//        _ASSERTE(Success);
//
//        auto InputPort = m_pPortSet->getInputPort("Main");
//        auto MergePort = m_pPortSet->getInputPort("Main");
//        m_pLuminanceImage = make<vk::CImage>();
//        m_pBlurredImage = make<vk::CImage>();
//        m_pOutputImage = make<vk::CImage>();
//        ImageUtils::createImage2d(*m_pLuminanceImage, m_pDevice, RefExtent, InputPort->getActualFormatV(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
//        ImageUtils::createImage2d(*m_pBlurredImage, m_pDevice, RefExtent, InputPort->getActualFormatV(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
//        ImageUtils::createImage2d(*m_pOutputImage, m_pDevice, RefExtent, MergePort->getActualFormatV(), VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
//        m_pPortSet->setOutput("Main", m_pOutputImage);
//
//        CRenderPassFullScreen::_initV();
//
//        m_LuminiancePipeline.create(m_pDevice, get(), RefExtent, 0);
//        m_BlurPipeline.create(m_pDevice, get(), RefExtent, 1);
//        m_MergePipeline.create(m_pDevice, get(), RefExtent, 2);
//        VkImageView InputImage = m_pPortSet->getInputPort("Main")->getImageV();
//        m_LuminiancePipeline.setInputImage(InputImage);
//        m_BlurPipeline.setInputImage(*m_pLuminanceImage);
//        m_MergePipeline.setInputImage(InputImage, *m_pBlurredImage);
//    }
//
//    virtual CPortSet::Ptr _createPortSetV() override
//    {
//        SPortDescriptor PortDesc;
//        PortDesc.addInput("Main", SPortInfo::createAnyOfUsage(EImageUsage::READ));
//        PortDesc.addOutput("Main", SPortInfo{ VK_FORMAT_B8G8R8A8_UNORM, SPortInfo::AnyExtent, 0, EImageUsage::COLOR_ATTACHMENT });
//        return make<CPortSet>(PortDesc);
//    }
//
//    void _destroyV() override
//    {
//        destroyAndClear(m_pLuminanceImage);
//        destroyAndClear(m_pBlurredImage);
//        destroyAndClear(m_pOutputImage);
//        m_LuminiancePipeline.destroy();
//        m_BlurPipeline.destroy();
//        m_MergePipeline.destroy();
//        CRenderPassFullScreen::_destroyV();
//    }
//
//    virtual CRenderPassDescriptor _getRenderPassDescV() override
//    {
//        CRenderPassDescriptor Desc;
//
//        Desc.addColorAttachment(m_pPortSet->getOutputPort("Main"));
//
//        VkFormat InputFormat = m_pPortSet->getInputPort("Main")->getActualFormatV();
//
//        SAttachementInfo LuminanceAttachementInfo;
//        LuminanceAttachementInfo.Format = InputFormat;
//        LuminanceAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        LuminanceAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        LuminanceAttachementInfo.IsBegin = true;
//        LuminanceAttachementInfo.IsEnd = false;
//        Desc.addColorAttachment(LuminanceAttachementInfo);
//
//        SAttachementInfo BlurredAttachementInfo;
//        BlurredAttachementInfo.Format = InputFormat;
//        BlurredAttachementInfo.InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        BlurredAttachementInfo.FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        BlurredAttachementInfo.IsBegin = true;
//        BlurredAttachementInfo.IsEnd = false;
//        Desc.addColorAttachment(BlurredAttachementInfo);
//        
//        Desc.addSubpass(SSubpassReferenceInfo()
//            .addColorRef(1)
//            .addDependentPass(VK_SUBPASS_EXTERNAL)
//        ); // luminance
//        Desc.addSubpass(SSubpassReferenceInfo()
//            .addColorRef(2)
//            .addInputRef(1)
//            .addDependentPass(0)
//        ); // blur
//        Desc.addSubpass(SSubpassReferenceInfo()
//            .addColorRef(0)
//            .addInputRef(2)
//            .addDependentPass(1)
//        ); // merge
//        return Desc;
//    }
//
//    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override
//    {
//        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();
//
//        _beginWithFramebuffer();
//
//        m_LuminiancePipeline.bind(pCommandBuffer);
//        _drawFullScreen(pCommandBuffer);
//        pCommandBuffer->goNextPass();
//        m_BlurPipeline.bind(pCommandBuffer);
//        _drawFullScreen(pCommandBuffer);
//        pCommandBuffer->goNextPass();
//        m_MergePipeline.bind(pCommandBuffer);
//        _drawFullScreen(pCommandBuffer);
//
//        _endWithFramebuffer();
//        return { pCommandBuffer->get() };
//    }
//
//    virtual std::vector<VkImageView> _getAttachmentsV() override
//    {
//        return
//        {
//            m_pPortSet->getOutputPort("Main")->getImageV(),
//            *m_pLuminanceImage,
//            *m_pBlurredImage
//        };
//    }
//
//    virtual std::vector<VkClearValue> _getClearValuesV() override
//    {
//        std::vector<VkClearValue> ValueSet(3);
//        ValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
//        ValueSet[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
//        ValueSet[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
//        return ValueSet;
//    }
//
//    virtual void _renderUIV() override
//    {
//        if (UI::collapse(u8"Bloom"))
//        {
//            //UI::drag(u8"模糊迭代次数", m_FilterIteration, 1, 1, 5); // TODO: implement multiple iter blur
//            UI::drag(u8"模糊范围", m_FilterSize, 2, 3, 15);
//            UI::drag(u8"强度", m_BloomFactor, 0.05f, 0.0f, 5.0f);
//        }
//    }
//
//    virtual void _updateV() override
//    {
//        m_BlurPipeline.updateUniformBuffer(uint32_t(m_FilterSize));
//        m_MergePipeline.updateUniformBuffer(m_BloomFactor);
//    }
//
//private:
//
//    vk::CImage::Ptr m_pLuminanceImage;
//    vk::CImage::Ptr m_pBlurredImage;
//    vk::CImage::Ptr m_pOutputImage;
//
//    CPipelineBloomLuminance m_LuminiancePipeline;
//    CPipelineBloomBlur m_BlurPipeline;
//    CPipelineBloomMerge m_MergePipeline;
//
//    float m_BloomFactor = 0.5f;
//    int m_FilterIteration = 3;
//    int m_FilterSize = 5;
//};
