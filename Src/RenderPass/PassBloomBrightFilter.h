#pragma once
#include "PassFullScreen.h"
#include "PipelineBloomBrightFilter.h"

class CRenderPassBlendBrightFilter : public CRenderPassFullScreenGeneral
{
protected:
    virtual ptr<IPipeline> _initPipelineV() override
    {
         auto pPipeline = make<CPipelineBlendBrightFilter>();
         pPipeline->setInputPort(m_pPortSet->getInputPort("Input"));
         return pPipeline;
    }

    virtual SPortDescriptor _getPortDescV() override
    {
        SPortDescriptor Ports;
        Ports.addInput("Input", SPortFormat::createAnyOfUsage(EUsage::READ));
        Ports.addOutput("Output", SPortFormat::createAnyOfUsage(EUsage::WRITE));
        return Ports;
    }

    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getInputPort("Input"));
    }

    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Output")->getImageV(vIndex)
        };
    }

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Input", voExtent);
    }

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override
    {
        VkExtent2D Extent;
        if (!_dumpReferenceExtentV(Extent)) return;

        if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
        {
            __createOutputImages(Extent);
        }

        CRenderPassFullScreenGeneral::_onUpdateV(vUpdateState);
    }

private:
    void __createOutputImages(VkExtent2D vExtent)
    {
        VkFormat Format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        uint32_t ImageNum = m_pAppInfo->getImageNum();
        m_BrightImageSet.init(ImageNum);
        for (uint32_t i = 0; i < ImageNum; ++i)
        {
            Function::createImage2d(*m_BrightImageSet[i], m_pDevice, vExtent, Format, Usage);

            m_pPortSet->setOutput("Output", *m_BrightImageSet[i]);
        }
    }

    vk::CPointerSet<vk::CImage> m_BrightImageSet;
};
