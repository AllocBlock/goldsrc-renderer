#pragma once
#include "ImageUtils.h"
#include "PassFullScreen.h"
#include "PipelineBloomLuminance.h"
#include "PipelineBloomBlur.h"
#include "PipelineBloomMerge.h"
#include <InterfaceGui.h>

// references:
// Unity Shader - Bloom(光晕、泛光): https://zhuanlan.zhihu.com/p/140724673
class CRenderPassBloom : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "Bloom";

protected:
    virtual void _initV() override
    {
        CRenderPassFullScreen::_initV();

        auto pInputPort = m_pPortSet->getInputPort("Main");
        auto Format = pInputPort->getImageV()->getFormat();
        m_pLuminanceImage = make<vk::CImage>();
        m_pBlurredImage = make<vk::CImage>();
        m_pOutputImage = make<vk::CImage>();
        ImageUtils::createImage2d(*m_pLuminanceImage, m_pDevice, m_ScreenExtent, Format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        ImageUtils::createImage2d(*m_pBlurredImage, m_pDevice, m_ScreenExtent, Format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        ImageUtils::createImage2d(*m_pOutputImage, m_pDevice, m_ScreenExtent, Format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        m_pPortSet->setOutput("Main", m_pOutputImage);

        m_PassLuminianceDescriptor.addColorAttachment(m_pLuminanceImage, true);
        m_LuminiancePipeline.create(m_pDevice, m_PassLuminianceDescriptor, m_ScreenExtent);
        m_LuminiancePipeline.setInputImage(*pInputPort->getImageV());

        m_PassBlurDescriptor.addColorAttachment(m_pBlurredImage, true);
        m_BlurPipeline.create(m_pDevice, m_PassBlurDescriptor, m_ScreenExtent);
        m_BlurPipeline.setInputImage(*m_pLuminanceImage);

        m_PassMergeDescriptor.addColorAttachment(m_pOutputImage, true);
        m_MergePipeline.create(m_pDevice, m_PassMergeDescriptor, m_ScreenExtent);
        m_MergePipeline.setInputImage(*pInputPort->getImageV(), *m_pBlurredImage);
    }

    virtual CPortSet::Ptr _createPortSetV() override
    {
        SPortDescriptor PortDesc;
        PortDesc.addInput("Main", SPortInfo::createAnyOfUsage(EImageUsage::READ));
        PortDesc.addOutput("Main", SPortInfo{ VK_FORMAT_B8G8R8A8_UNORM, SPortInfo::AnyExtent, 0, EImageUsage::COLOR_ATTACHMENT });
        return make<CPortSet>(PortDesc);
    }

    void _destroyV() override
    {
        destroyAndClear(m_pLuminanceImage);
        destroyAndClear(m_pBlurredImage);
        destroyAndClear(m_pOutputImage);
        m_LuminiancePipeline.destroy();
        m_BlurPipeline.destroy();
        m_MergePipeline.destroy();
        CRenderPassFullScreen::_destroyV();
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();

        _beginCommand(pCommandBuffer);

        m_pLuminanceImage->transitionLayout(pCommandBuffer, m_pLuminanceImage->getLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        // 1. calculate luminiance
        {
            _beginRendering(pCommandBuffer, m_PassLuminianceDescriptor.generateRendererInfo(m_ScreenExtent));
            m_LuminiancePipeline.bind(pCommandBuffer);
            _drawFullScreen(pCommandBuffer);
            _endRendering();
        }

        // 2. blur luminiance
        m_pLuminanceImage->transitionLayout(pCommandBuffer, m_pLuminanceImage->getLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_pBlurredImage->transitionLayout(pCommandBuffer, m_pBlurredImage->getLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        {
            _beginRendering(pCommandBuffer, m_PassBlurDescriptor.generateRendererInfo(m_ScreenExtent));
            m_BlurPipeline.bind(pCommandBuffer);
            _drawFullScreen(pCommandBuffer);
            _endRendering();
        }

        // 3. apply blur to main image
        m_pBlurredImage->transitionLayout(pCommandBuffer, m_pBlurredImage->getLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        {

            _beginRendering(pCommandBuffer, m_PassMergeDescriptor.generateRendererInfo(m_ScreenExtent));
            m_MergePipeline.bind(pCommandBuffer);
            _drawFullScreen(pCommandBuffer);
            _endRendering();
        }

        _endCommand();
        return { pCommandBuffer->get() };
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

    virtual void _updateV() override
    {
        m_BlurPipeline.updateUniformBuffer(uint32_t(m_FilterSize));
        m_MergePipeline.updateUniformBuffer(m_BloomFactor);
    }

private:

    vk::CImage::Ptr m_pLuminanceImage;
    vk::CImage::Ptr m_pBlurredImage;
    vk::CImage::Ptr m_pOutputImage;

    CRenderInfoDescriptor m_PassLuminianceDescriptor;
    CRenderInfoDescriptor m_PassBlurDescriptor;
    CRenderInfoDescriptor m_PassMergeDescriptor;

    CPipelineBloomLuminance m_LuminiancePipeline;
    CPipelineBloomBlur m_BlurPipeline;
    CPipelineBloomMerge m_MergePipeline;

    float m_BloomFactor = 0.5f;
    int m_FilterIteration = 3;
    int m_FilterSize = 5;
};
