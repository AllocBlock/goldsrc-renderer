#include "RenderPass.h"

using namespace vk;

IRenderPass::IRenderPass()
{
}

void IRenderPass::createPortSet()
{
    m_pPortSet = _createPortSetV();
}

void IRenderPass::init(vk::CDevice::CPtr vDevice, size_t vImageNum, VkExtent2D vScreenExtent)
{
    _ASSERTE(vImageNum > 0);
    _ASSERTE(vScreenExtent.width > 0 && vScreenExtent.height > 0);
    m_pDevice = vDevice;
    m_ImageNum = vImageNum;
    m_ScreenExtent = vScreenExtent;
    m_pPortSet->assertInputLinkReady();
    
    __createRenderpass();
    _initV();
}

void IRenderPass::update(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    _updateV(vImageIndex);
}

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    return _requestCommandBuffersV(vImageIndex);
}

void IRenderPass::destroy()
{
    _destroyV();
    __destroyRenderpass();
    m_pPortSet->unlinkAll();
}

void IRenderPass::_begin(CCommandBuffer::Ptr vCommandBuffer, CFrameBuffer::CPtr vFrameBuffer, const std::vector<VkClearValue>& vClearValues, bool vHasSecondary)
{
    _ASSERTE(!m_Begined);
    _ASSERTE(isValid());
    if (m_pCurrentCommandBuffer != nullptr)
        throw std::runtime_error("Already begun with another command buffer");

    // only need one command one pass, so begin/end command buffer at same time with renderpass
    vCommandBuffer->begin();

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = get();
    RenderPassBeginInfo.framebuffer = *vFrameBuffer;
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = vFrameBuffer->getExtent();
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(vClearValues.size());
    RenderPassBeginInfo.pClearValues = vClearValues.data();

    vCommandBuffer->beginRenderPass(RenderPassBeginInfo, vHasSecondary);

    m_pCurrentCommandBuffer = vCommandBuffer;
    m_Begined = true;
}

void IRenderPass::_end()
{
    _ASSERTE(m_Begined);
    m_pCurrentCommandBuffer->endRenderPass();
    m_pCurrentCommandBuffer->end();
    m_pCurrentCommandBuffer = nullptr;
    m_Begined = false;
}

bool IRenderPass::_dumpInputPortExtent(std::string vName, VkExtent2D& voExtent)
{
    bool HasExtent = false;
    auto pRefPort = m_pPortSet->getInputPort(vName);
    if (pRefPort->hasActualExtentV())
    {
        HasExtent = true;
        voExtent = pRefPort->getActualExtentV();
    }
    return HasExtent;
}

void IRenderPass::__createRenderpass()
{
    CRenderPassDescriptor OldDesc = m_CurPassDesc;
    m_CurPassDesc = _getRenderPassDescV();
    if (OldDesc == m_CurPassDesc) return; // same, no change
    if (!OldDesc.isValid() && !m_CurPassDesc.isValid()) return; // all invalid, no change

    __destroyRenderpass();
    _ASSERTE(m_CurPassDesc.isValid());

    auto Info = m_CurPassDesc.generateInfo();
    vk::checkError(vkCreateRenderPass(*m_pDevice, &Info, nullptr, _getPtr()));
    m_CurPassDesc.clearStage(); // free stage data to save memory

    Log::logCreation(std::string(typeid(*this).name()) + "(renderpass)", uint64_t(get()));
}

void IRenderPass::__destroyRenderpass()
{
    if (isValid())
    {
        vkDestroyRenderPass(*m_pDevice, get(), nullptr);
        _setNull();
    }
}
