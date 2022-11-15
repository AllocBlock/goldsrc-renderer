#include "PassOutlineEdge.h"
#include "Common.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"

#include <vector>

void CRenderPassOutlineEdge::_initV()
{
    CRenderPassSingle::_initV();

    __createVertexBuffer();
}

void CRenderPassOutlineEdge::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    vioDesc.addInput("Mask", SPortFormat::createAnyOfUsage(EUsage::READ));
}

CRenderPassDescriptor CRenderPassOutlineEdge::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassOutlineEdge::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    CRenderPassSingle::_onUpdateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (!_dumpReferenceExtentV(RefExtent)) return;

    if (vUpdateState.RenderpassUpdated || vUpdateState.InputImageUpdated || vUpdateState.ImageNum.IsUpdated || vUpdateState.ScreenExtent.IsUpdated)
    {
        if (isValid())
        {
            if (!vUpdateState.InputImageUpdated)
            {
                m_Pipeline.create(m_pDevice, get(), RefExtent);
                m_Pipeline.setImageNum(m_pAppInfo->getImageNum());
            }
        }
    }
}

std::vector<VkCommandBuffer> CRenderPassOutlineEdge::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_Pipeline.isValid());

    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    // FIXME: is update descriptor every frame slow?
    m_Pipeline.setInputImage(m_pPortSet->getInputPort("Mask")->getImageV(vImageIndex), vImageIndex);

    _beginWithFramebuffer(vImageIndex);
    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }

    _endWithFramebuffer();
    return { CommandBuffer };
}

void CRenderPassOutlineEdge::_destroyV()
{
    m_Pipeline.destroy();
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingle::_destroyV();
}

void CRenderPassOutlineEdge::__createVertexBuffer()
{
    m_PointDataSet =
    {
        CPipelineEdge::SPointData{glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)},
        CPipelineEdge::SPointData{glm::vec2( 3.0f, -1.0f), glm::vec2(2.0f, 0.0f)},
        CPipelineEdge::SPointData{glm::vec2(-1.0f,  3.0f), glm::vec2(0.0f, 2.0f)},
    };
    
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(CPipelineEdge::SPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}
