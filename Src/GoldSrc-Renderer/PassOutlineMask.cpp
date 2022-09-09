#include "PassOutlineMask.h"
#include "Common.h"
#include "Descriptor.h"
#include "Function.h"
#include "Gui.h"
#include "RenderPassDescriptor.h"

#include <vector>

void COutlineMaskRenderPass::setHighlightObject(ptr<C3DObjectGoldSrc> vObject)
{
    m_PipelineMask.setObject(vObject);
    __rerecordCommand();
}

void COutlineMaskRenderPass::removeHighlight()
{
    m_PipelineMask.removeObject();
    __rerecordCommand();
}

void COutlineMaskRenderPass::_initV()
{
    IRenderPass::_initV();
    m_pPortSet->getInputPort("Depth")->hookImageUpdate([this]()
        {
            if (isValid())
            {
                __createFramebuffers();
            }
        });
    __rerecordCommand();
}

SPortDescriptor COutlineMaskRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInput("Main"); // FIXME: only need extent and update trigger, but not overwrite
    Ports.addInput("Depth", { VK_FORMAT_D32_SFLOAT, m_AppInfo.Extent, 1 });
    Ports.addOutput("Mask", { VK_FORMAT_R8G8B8A8_UNORM, {0, 0}, 0});
    return Ports;
}

CRenderPassDescriptor COutlineMaskRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Mask"),
                                                            m_pPortSet->getInputPort("Depth"));
}

void COutlineMaskRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createMaskImage();
        __createFramebuffers();
        if (isValid())
        {
            m_PipelineMask.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
            m_PipelineMask.setImageNum(m_AppInfo.ImageNum);
        }

        __rerecordCommand();
    }
}

void COutlineMaskRenderPass::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pCamera);
    m_PipelineMask.updateUniformBuffer(vImageIndex, m_pCamera);
}

void COutlineMaskRenderPass::_renderUIV()
{
}

void COutlineMaskRenderPass::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    m_MaskImageSet.destroyAndClearAll();
    m_PipelineMask.destroy();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> COutlineMaskRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
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

void COutlineMaskRenderPass::__rerecordCommand()
{
    m_RerecordCommandTimes += m_AppInfo.ImageNum;
}

void COutlineMaskRenderPass::__createMaskImage()
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

void COutlineMaskRenderPass::__createFramebuffers()
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
            m_pPortSet->getInputPort("Depth")->getImageV(),
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}