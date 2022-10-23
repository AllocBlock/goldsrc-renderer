#include "PassOutlineMask.h"
#include "Common.h"
#include "InterfaceUI.h"

#include <vector>

void CRenderPassOutlineMask::setHighlightActor(CActor<CMeshDataGoldSrc>::Ptr vActor)
{
    m_PipelineMask.setActor(vActor);
    __rerecordCommand();
}

void CRenderPassOutlineMask::removeHighlight()
{
    m_PipelineMask.removeObject();
    __rerecordCommand();
}

void CRenderPassOutlineMask::_initV()
{
    IRenderPass::_initV();
    __rerecordCommand();
}

SPortDescriptor CRenderPassOutlineMask::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInput("Main", SPortFormat::createAnyOfUsage(EUsage::READ));
    Ports.addOutput("Mask", { VK_FORMAT_R8G8B8A8_UNORM, {0, 0}, 0, EUsage::WRITE });
    return Ports;
}

CRenderPassDescriptor CRenderPassOutlineMask::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Mask"));
}

void CRenderPassOutlineMask::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createMaskImage();
        if (isValid())
        {
            __createFramebuffers();
            m_PipelineMask.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
            m_PipelineMask.setImageNum(m_AppInfo.ImageNum);
        }

        __rerecordCommand();
    }
}

void CRenderPassOutlineMask::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pCamera);
    m_PipelineMask.updateUniformBuffer(vImageIndex, m_pCamera);
}

void CRenderPassOutlineMask::_renderUIV()
{
}

void CRenderPassOutlineMask::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    m_MaskImageSet.destroyAndClearAll();
    m_PipelineMask.destroy();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassOutlineMask::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

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

        begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);
        m_PipelineMask.recordCommand(CommandBuffer, vImageIndex);
        end();
    }
    return { CommandBuffer };
}

void CRenderPassOutlineMask::__rerecordCommand()
{
    m_RerecordCommandTimes = m_AppInfo.ImageNum;
}

void CRenderPassOutlineMask::__createMaskImage()
{
    VkFormat Format = m_pPortSet->getOutputFormat("Mask").Format;

    m_MaskImageSet.destroyAndClearAll();
    m_MaskImageSet.init(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        VkImageCreateInfo ImageInfo = {};
        ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ImageInfo.imageType = VK_IMAGE_TYPE_2D;
        ImageInfo.extent.width = static_cast<uint32_t>(m_AppInfo.Extent.width);
        ImageInfo.extent.height = static_cast<uint32_t>(m_AppInfo.Extent.height);
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

        m_MaskImageSet[i]->create(m_AppInfo.pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);

        m_pPortSet->setOutput("Mask", *m_MaskImageSet[i], i);
    }
}

void CRenderPassOutlineMask::__createFramebuffers()
{
    if (!isValid()) return;

    m_AppInfo.pDevice->waitUntilIdle();
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Mask")->getImageV(i),
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}
