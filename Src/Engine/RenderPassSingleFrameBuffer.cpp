#include "RenderPassSingleFrameBuffer.h"

CPortSet::Ptr CRenderPassSingleFrameBuffer::_createPortSetV()
{
    SPortDescriptor PortDesc;
    _initPortDescV(PortDesc);
    return make<CPortSet>(PortDesc);
}

void CRenderPassSingleFrameBuffer::_initV()
{
    __createCommandPoolAndBuffers(m_ImageNum);

    m_ClearValueSet = _getClearValuesV();

    VkExtent2D Extent = vk::ZeroExtent;
    bool Success = _dumpReferenceExtentV(Extent);
    _ASSERTE(Success);
    __createFramebuffers(Extent);
}

void CRenderPassSingleFrameBuffer::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    __destroyCommandPoolAndBuffers();
}

std::vector<std::string> CRenderPassSingleFrameBuffer::_getExtraCommandBufferNamesV() const
{ return {}; }

CCommandBuffer::Ptr CRenderPassSingleFrameBuffer::_getCommandBuffer(uint32_t vImageIndex)
{
    return m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);
}

void CRenderPassSingleFrameBuffer::_beginWithFramebuffer(uint32_t vImageIndex, bool vHasSecondary)
{
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    CCommandBuffer::Ptr CommandBuffer = _getCommandBuffer(vImageIndex);

    _ASSERTE(m_FramebufferSet[vImageIndex]->getAttachmentNum() == m_ClearValueSet.size());
    _begin(CommandBuffer, m_FramebufferSet[vImageIndex], m_ClearValueSet, vHasSecondary);
}

void CRenderPassSingleFrameBuffer::_endWithFramebuffer()
{
    _end();
}

void CRenderPassSingleFrameBuffer::_beginSecondary(CCommandBuffer::Ptr vCommandBuffer, uint32_t vImageIndex)
{
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));
    vCommandBuffer->beginSecondary(get(), 0, *m_FramebufferSet[vImageIndex]);
}

void CRenderPassSingleFrameBuffer::__createCommandPoolAndBuffers(uint32_t vImageNum)
{
    __destroyCommandPoolAndBuffers();
    if (vImageNum == 0) return;
    m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, static_cast<uint32_t>(vImageNum), ECommandBufferLevel::PRIMARY);

    for (const auto& Name : _getExtraCommandBufferNamesV())
    {
        m_Command.createBuffers(Name, static_cast<uint32_t>(vImageNum), ECommandBufferLevel::SECONDARY);
    }
}

void CRenderPassSingleFrameBuffer::__destroyCommandPoolAndBuffers()
{
    m_Command.clear();
}

void CRenderPassSingleFrameBuffer::__createFramebuffers(VkExtent2D vExtent)
{
    _ASSERTE(isValid());
    m_pPortSet->assertReady();

    m_FramebufferSet.destroyAndClearAll();
    
    m_FramebufferSet.init(m_ImageNum);
    for (uint32_t i = 0; i < m_ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet = _getAttachmentsV(i);
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
}
