#include "PassOutlineMask.h"
#include "Common.h"
#include "InterfaceUI.h"

#include <vector>

void CRenderPassOutlineMask::setHighlightActor(CActor::Ptr vActor)
{
    m_PipelineCreator.get().setActor(vActor);
    __rerecordCommand();
}

void CRenderPassOutlineMask::removeHighlight()
{
    m_PipelineCreator.get().removeObject();
    __rerecordCommand();
}

void CRenderPassOutlineMask::_initV()
{
    CRenderPassSingle::_initV();

    m_MaskImageCreator.init(m_pAppInfo->getScreenExtent(), true, 
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            VkFormat Format = m_pPortSet->getOutputFormat("Mask").Format;
            
            vImageSet.init(m_pAppInfo->getImageNum());
            for (size_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
            {
                VkImageCreateInfo ImageInfo = {};
                ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                ImageInfo.imageType = VK_IMAGE_TYPE_2D;
                ImageInfo.extent.width = static_cast<uint32_t>(vExtent.width);
                ImageInfo.extent.height = static_cast<uint32_t>(vExtent.height);
                ImageInfo.extent.depth = 1;
                ImageInfo.mipLevels = 1;
                ImageInfo.arrayLayers = 1;
                ImageInfo.format = Format;
                ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                vk::SImageViewInfo ViewInfo;
                ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

                vImageSet[i]->create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);

                m_pPortSet->setOutput("Mask", *vImageSet[i], i);
            }
        }
    );

    m_PipelineCreator.init(m_pDevice, weak_from_this(), m_pAppInfo->getScreenExtent(), true, m_pAppInfo->getImageNum(),
        [this](IPipeline& vPipeline) { __rerecordCommand(); }
    );

    __rerecordCommand();
}

void CRenderPassOutlineMask::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addOutput("Mask", { VK_FORMAT_R8G8B8A8_UNORM, {0, 0}, 0, EUsage::WRITE });
}

CRenderPassDescriptor CRenderPassOutlineMask::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Mask"));
}

void CRenderPassOutlineMask::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_MaskImageCreator.updateV(vUpdateState);
    m_PipelineCreator.updateV(vUpdateState);

    CRenderPassSingle::_onUpdateV(vUpdateState);
}

void CRenderPassOutlineMask::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pCamera);
    m_PipelineCreator.get().updateUniformBuffer(vImageIndex, m_pCamera);
}

void CRenderPassOutlineMask::_renderUIV()
{
}

void CRenderPassOutlineMask::_destroyV()
{
    m_MaskImageCreator.destroy();
    m_PipelineCreator.destroy();

    CRenderPassSingle::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassOutlineMask::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    bool RerecordCommand = false;
    if (m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        // init
        std::vector<VkClearValue> ClearValueSet(2);
        ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValueSet[1].depthStencil = { 1.0f, 0 };

        _beginWithFramebuffer(vImageIndex);
        m_PipelineCreator.get().recordCommand(CommandBuffer, vImageIndex);
        _endWithFramebuffer();
    }
    return { CommandBuffer };
}

void CRenderPassOutlineMask::__rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}
