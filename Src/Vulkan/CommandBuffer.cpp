#include "PchVulkan.h"
#include "CommandBuffer.h"

CCommandBuffer::CCommandBuffer(VkCommandBuffer vBuffer, bool vIsSingleTimeBuffer, ECommandBufferLevel level)
{
    if (vBuffer == VK_NULL_HANDLE)
        throw std::runtime_error("Command buffer is invalid.");
    m_Buffer = vBuffer;
    m_IsSingleTimeBuffer = vIsSingleTimeBuffer;
    m_IsSecondary = level == ECommandBufferLevel::SECONDARY;
}

bool CCommandBuffer::isValid()
{
    return m_Buffer != VK_NULL_HANDLE;
}

VkCommandBuffer CCommandBuffer::get()
{
    return m_Buffer;
}

void CCommandBuffer::begin()
{
    _ASSERTE(!m_IsSecondary);
    __assertValid();
    if (m_IsBegun)
        throw std::runtime_error("Already begun");

    m_IsBegun = true;

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCommandBufferUsageFlags Usage = m_IsSingleTimeBuffer ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    CommandBufferBeginInfo.flags = Usage;

    vk::checkError(vkBeginCommandBuffer(m_Buffer, &CommandBufferBeginInfo));
}

void CCommandBuffer::beginSecondary(VkRenderPass vPass, uint32_t vSubPass, VkFramebuffer vFramebuffer)
{
    _ASSERTE(m_IsSecondary);
    __assertValid();
    if (m_IsBegun)
        throw std::runtime_error("Already begun");

    m_IsBegun = true;

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkCommandBufferInheritanceInfo InheritInfo = {};
    InheritInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    InheritInfo.renderPass = vPass;
    InheritInfo.subpass = vSubPass;
    InheritInfo.framebuffer = vFramebuffer;
    
    CommandBufferBeginInfo.pInheritanceInfo = &InheritInfo;

    vk::checkError(vkBeginCommandBuffer(m_Buffer, &CommandBufferBeginInfo));
}

void CCommandBuffer::end()
{
    __assertValid();
    if (!m_IsBegun)
        throw std::runtime_error("Should call begin before end");
    if (m_IsPassBegun)
        throw std::runtime_error("Should end pass beforn end record");

    m_IsBegun = false;
    m_BoundVertBuffer = VK_NULL_HANDLE;
    m_BoundPipeline = VK_NULL_HANDLE;
    m_BoundPipelineLayout = VK_NULL_HANDLE;
    vk::checkError(vkEndCommandBuffer(m_Buffer));
}

void CCommandBuffer::__assertValid()
{
    if (!isValid())
        throw std::runtime_error("Command buffer is not valid.");
}
