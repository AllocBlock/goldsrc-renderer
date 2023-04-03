#include "PassOutlineEdge.h"
#include "Common.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"

#include <vector>

void CRenderPassOutlineEdge::_initV()
{
    CRenderPassSingle::_initV();

    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);

    m_PipelineCreator.init(m_pDevice, weak_from_this(), RefExtent, false, m_pAppInfo->getImageNum());
    
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
    m_PipelineCreator.updateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (_dumpReferenceExtentV(RefExtent))
    {
        m_PipelineCreator.updateExtent(RefExtent);
    }

    CRenderPassSingle::_onUpdateV(vUpdateState);
}

std::vector<VkCommandBuffer> CRenderPassOutlineEdge::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_PipelineCreator.isReady());

    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    // FIXME: is update descriptor every frame slow?
    auto& Pipeline = m_PipelineCreator.get();
    Pipeline.setInputImage(m_pPortSet->getInputPort("Mask")->getImageV(vImageIndex), vImageIndex);

    _beginWithFramebuffer(vImageIndex);
    if (m_pVertexBuffer->isValid())
    {
        pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
        Pipeline.bind(pCommandBuffer, vImageIndex);
        pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
    }

    _endWithFramebuffer();
    return { pCommandBuffer->get() };
}

void CRenderPassOutlineEdge::_destroyV()
{
    m_PipelineCreator.destroy();
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingle::_destroyV();
}

void CRenderPassOutlineEdge::__createVertexBuffer()
{
    m_PointDataSet =
    {
        CPipelineEdge::SPointData{glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)},
        CPipelineEdge::SPointData{glm::vec2(-1.0f,  3.0f), glm::vec2(0.0f, 2.0f)},
        CPipelineEdge::SPointData{glm::vec2( 3.0f, -1.0f), glm::vec2(2.0f, 0.0f)},
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
