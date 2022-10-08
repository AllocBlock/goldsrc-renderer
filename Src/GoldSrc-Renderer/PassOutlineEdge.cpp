#include "PassOutlineEdge.h"
#include "Common.h"
#include "Descriptor.h"
#include "Gui.h"
#include "RenderPassDescriptor.h"

#include <vector>

void COutlineEdgeRenderPass::_initV()
{
    __createVertexBuffer();
}

SPortDescriptor COutlineEdgeRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    Ports.addInput("Mask", SPortFormat::createAnyOfUsage(EUsage::READ));
    return Ports;
}

CRenderPassDescriptor COutlineEdgeRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void COutlineEdgeRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createFramebuffers();

        if ((vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated) && isValid())
        {
            m_Pipeline.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
        }

        if (m_Pipeline.isValid())
            m_Pipeline.setImageNum(m_AppInfo.ImageNum);
    }
}

std::vector<VkCommandBuffer> COutlineEdgeRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_Pipeline.isValid());
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // FIXME: is update descriptor every frame slow?
    m_Pipeline.setInputImage(m_pPortSet->getInputPort("Mask")->getImageV(vImageIndex), vImageIndex);

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, { ClearValue });
    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }

    end();
    return { CommandBuffer };
}

void COutlineEdgeRenderPass::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    m_Pipeline.destroy();
    destroyAndClear(m_pVertexBuffer);

    IRenderPass::_destroyV();
}

void COutlineEdgeRenderPass::__createFramebuffers()
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i)
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void COutlineEdgeRenderPass::__createVertexBuffer()
{
    m_PointDataSet =
    {
        CPipelineEdge::SPointData{glm::vec2(-1.0f, -1.0f), glm::vec2(0.0, 0.0)},
        CPipelineEdge::SPointData{glm::vec2(3.0f, -1.0f), glm::vec2(2.0, 0.0)},
        CPipelineEdge::SPointData{glm::vec2(-1.0f, 3.0f), glm::vec2(0.0, 2.0)},
    };
    
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(CPipelineEdge::SPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}