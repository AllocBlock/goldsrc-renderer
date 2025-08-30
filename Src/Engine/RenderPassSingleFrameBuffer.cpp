#include "RenderPassSingleFrameBuffer.h"

CPortSet::Ptr CRenderPassSingleFrameBuffer::_createPortSetV()
{
    SPortDescriptor PortDesc;
    _initPortDescV(PortDesc);
    return make<CPortSet>(PortDesc);
}

void CRenderPassSingleFrameBuffer::_initV()
{
    __createCommandPoolAndBuffers();

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

CCommandBuffer::Ptr CRenderPassSingleFrameBuffer::_getCommandBuffer()
{
    return m_Command.getCommandBuffer(m_DefaultCommandName);
}

void CRenderPassSingleFrameBuffer::_beginWithFramebuffer(bool vHasSecondary)
{
    _ASSERTE(m_FramebufferSet.isValid(0));

    CCommandBuffer::Ptr CommandBuffer = _getCommandBuffer();

    _ASSERTE(m_FramebufferSet[0]->getAttachmentNum() == m_ClearValueSet.size());
    _begin(CommandBuffer, m_FramebufferSet[0], m_ClearValueSet, vHasSecondary);
}

void CRenderPassSingleFrameBuffer::_endWithFramebuffer()
{
    _end();
}

void CRenderPassSingleFrameBuffer::_beginSecondary(CCommandBuffer::Ptr vCommandBuffer)
{
    _ASSERTE(m_FramebufferSet.isValid(0));
    vCommandBuffer->beginSecondary(get(), 0, *m_FramebufferSet[0]);
}

void CRenderPassSingleFrameBuffer::__createCommandPoolAndBuffers()
{
    __destroyCommandPoolAndBuffers();
    m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, ECommandBufferLevel::PRIMARY);

    for (const auto& Name : _getExtraCommandBufferNamesV())
    {
        m_Command.createBuffers(Name, ECommandBufferLevel::SECONDARY);
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
    
    m_FramebufferSet.init(1);
    for (uint32_t i = 0; i < 1; ++i)
    {
        std::vector<VkImageView> AttachmentSet = _getAttachmentsV();
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
}
