#include "RenderPass.h"

using namespace engine;

IRenderPass::IRenderPass()
{
}

void IRenderPass::createPortSet()
{
    m_pPortSet = _createPortSetV();
}

void IRenderPass::init(cptr<vk::CDevice> vDevice, VkExtent2D vScreenExtent)
{
    _ASSERTE(vScreenExtent.width > 0 && vScreenExtent.height > 0);
    m_pDevice = vDevice;
    m_ScreenExtent = vScreenExtent;
    m_pPortSet->assertInputReady();

    __createCommandPoolAndBuffers();

    _initV();
}

void IRenderPass::update()
{
    _updateV();
}

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers()
{
    return _requestCommandBuffersV();
}

void IRenderPass::destroy()
{
    _destroyV();
    __destroyCommandPoolAndBuffers();
    m_pPortSet->unlinkAll();
}

void IRenderPass::_beginCommand(sptr<CCommandBuffer> vCommandBuffer)
{
    _ASSERTE(!m_CommandBegun);
    if (m_pCurrentCommandBuffer != nullptr)
        throw std::runtime_error("Already begun with another command buffer");
    m_pCurrentCommandBuffer = vCommandBuffer;

    m_pCurrentCommandBuffer->begin();
    _initImageLayouts(vCommandBuffer);

    m_CommandBegun = true;
}

void IRenderPass::_beginRendering(sptr<CCommandBuffer> vCommandBuffer, const VkRenderingInfo& vBeginInfo)
{
    _ASSERTE(m_CommandBegun);
    m_pCurrentCommandBuffer->beginRendering(vBeginInfo);
}

void IRenderPass::_addPassBarrier(sptr<CCommandBuffer> vCommandBuffer)
{
    // 比较暴力的Pass级别的同步，应该用不到，用在做Image Layout转换时顺便做Image级别的同步就ok了
    _ASSERTE(m_CommandBegun);
    vCommandBuffer->addMemoryBarrier(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT
    );
}

void IRenderPass::_endRendering()
{
    _ASSERTE(m_CommandBegun);
    m_pCurrentCommandBuffer->endRendering();
}

void IRenderPass::_endCommand()
{
    _ASSERTE(m_CommandBegun);
    m_pCurrentCommandBuffer->end();
    m_pCurrentCommandBuffer = nullptr;
    m_CommandBegun = false;
}

void IRenderPass::_beginSecondaryCommand(sptr<CCommandBuffer> vCommandBuffer, const CRenderInfoDescriptor& vRenderInfoDescriptor)
{
    std::vector<VkFormat> ColorAttachmentFormats = vRenderInfoDescriptor.getColorAttachmentFormats();

    VkCommandBufferInheritanceRenderingInfo InheritRenderingInfo = {};
    InheritRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
    InheritRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachmentFormats.size());
    InheritRenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats.data();
    if (vRenderInfoDescriptor.hasDepthAttachment())
    {
        InheritRenderingInfo.depthAttachmentFormat = vRenderInfoDescriptor.getDepthAttachmentFormat();
        InheritRenderingInfo.stencilAttachmentFormat = vRenderInfoDescriptor.getDepthAttachmentFormat();
    }
    InheritRenderingInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

    vCommandBuffer->beginSecondary(InheritRenderingInfo);
}

bool IRenderPass::_dumpInputPortExtent(std::string vName, VkExtent2D& voExtent)
{
    bool HasExtent = false;
    auto pRefPort = m_pPortSet->getInputPort(vName);
    auto pImage = pRefPort->getImageV();
    if (pImage)
    {
        HasExtent = true;
        voExtent = pImage->getExtent();
    }
    return HasExtent;
}


sptr<CCommandBuffer> IRenderPass::_getCommandBuffer()
{
    return m_Command.getCommandBuffer(m_DefaultCommandName);
}

void IRenderPass::_initImageLayouts(sptr<CCommandBuffer> vCommandBuffer)
{
    for (int i = 0; i < m_pPortSet->getInputPortNum(); ++i)
    {
        const auto& pPort = m_pPortSet->getInputPort(i);
        auto pImage = pPort->getImageV();
        if (pPort->getLayout() != pImage->getLayout())
        {
            pImage->transitionLayout(vCommandBuffer, pImage->getLayout(), pPort->getLayout());
        }
    }

    for (int i = 0; i < m_pPortSet->getOutputPortNum(); ++i)
    {
        const auto& pPort = m_pPortSet->getOutputPort(i);
        auto pImage = pPort->getImageV();
        if (pPort->getLayout() != pImage->getLayout())
        {
            pImage->transitionLayout(vCommandBuffer, pImage->getLayout(), pPort->getLayout());
        }
    }
}

void IRenderPass::__createCommandPoolAndBuffers()
{
    __destroyCommandPoolAndBuffers();
    m_Command.createPool(m_pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, ECommandBufferLevel::PRIMARY);

    // secondary
    for (const auto& Name : _getSecondaryCommandBufferNamesV())
    {
        m_Command.createBuffers(Name, ECommandBufferLevel::SECONDARY);
    }

    const std::type_info& TypeInfo = typeid(*this);
    m_Command.setDebugName(TypeInfo.name());
}

void IRenderPass::__destroyCommandPoolAndBuffers()
{
    m_Command.clear();
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
