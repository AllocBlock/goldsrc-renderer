#include "RenderPass.h"

using namespace vk;

IRenderPass::IRenderPass()
{
}

void IRenderPass::createPortSet()
{
    m_pPortSet = _createPortSetV();
}

void IRenderPass::init(vk::CDevice::CPtr vDevice, VkExtent2D vScreenExtent)
{
    _ASSERTE(vScreenExtent.width > 0 && vScreenExtent.height > 0);
    m_pDevice = vDevice;
    m_ScreenExtent = vScreenExtent;
    m_pPortSet->assertInputReady();
    
    __createRenderpass();
    _ASSERTE(isValid());
    _initV();
}

void IRenderPass::update()
{
    _ASSERTE(isValid());
    _updateV();
}

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers()
{
    _ASSERTE(isValid());
    return _requestCommandBuffersV();
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

std::string __toString(VkImageLayout vLayout)
{

    switch (vLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED: return "Undefined";
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return "Transfer Read";
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return "Transfer Write";
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return "Shader Read";
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return "Color Attachment";
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return "Depth Attachment";
    default:
        throw std::runtime_error("Unsupported layout");
    }
}

void IRenderPass::__createRenderpass()
{
    __destroyRenderpass();
    CRenderPassDescriptor PassDesc = _getRenderPassDescV();

    auto Info = PassDesc.generateInfo();
    vk::checkError(vkCreateRenderPass(*m_pDevice, &Info, nullptr, _getPtr()));
    PassDesc.clearStage(); // free stage data to save memory

#ifdef  _DEBUG
    Log::logCreation(std::string(typeid(*this).name()) + "(renderpass)", uint64_t(get()));
    Log::log("\tInput port:");
    for (size_t i = 0; i < m_pPortSet->getInputPortNum(); ++i)
    {
        auto pPort = m_pPortSet->getInputPort(i);
        Log::log("\t\t" + pPort->getName() + ":\t" + __toString(pPort->getInputLayout()) + "\t->\t" + __toString(pPort->getOutputLayout()));
    }
    Log::log("\tOutput port:");
    for (size_t i = 0; i < m_pPortSet->getInputPortNum(); ++i)
    {
        auto pPort = m_pPortSet->getInputPort(i);
        Log::log("\t\t" + pPort->getName() + ":\t" + __toString(pPort->getInputLayout()) + "\t->\t" + __toString(pPort->getOutputLayout()));
    }
#endif
}

void IRenderPass::__destroyRenderpass()
{
    if (isValid())
    {
        vkDestroyRenderPass(*m_pDevice, get(), nullptr);
        _setNull();
    }
}
