#include "PassGUI.h"
#include "InterfaceGui.h"

const std::string gChineseFont = "C:/windows/fonts/simhei.ttf";

CRenderPassGUI::CRenderPassGUI(GLFWwindow* vWindow)
{
    m_pWindow = vWindow;
}

void CRenderPassGUI::_initV()
{
    __createDescriptorPool();

    m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    std::vector<VkFormat> ColorAttachmentFormats = m_RenderInfoDescriptor.getColorAttachmentFormats();

    VkPipelineRenderingCreateInfoKHR DynamicRenderingInfo = {};
    DynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    DynamicRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachmentFormats.size());
    DynamicRenderingInfo.pColorAttachmentFormats = ColorAttachmentFormats.data();
    DynamicRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    DynamicRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    
    UI::init(m_pDevice, m_pWindow, m_DescriptorPool, DynamicRenderingInfo);
    CCommandBuffer::Ptr pCommandBuffer = m_Command.beginSingleTimeBuffer();
    UI::setFont(gChineseFont, pCommandBuffer);
    m_Command.endSingleTimeBuffer(pCommandBuffer);
}

CPortSet::Ptr CRenderPassGUI::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    return make<CPortSet>(PortDesc);
}

void CRenderPassGUI::_renderUIV()
{
    UI::showDebugLogWindow();
}

void CRenderPassGUI::_destroyV()
{
    if (UI::isInitted())
        UI::destory();
    __destroyDescriptorPool();
    m_pWindow = nullptr;
}

std::vector<VkCommandBuffer> CRenderPassGUI::_requestCommandBuffersV()
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();

    _beginCommand(pCommandBuffer);
    _beginRendering(pCommandBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent));
    UI::draw(pCommandBuffer);
    _endRendering();
    _endCommand();

    return { pCommandBuffer->get() };
}

void CRenderPassGUI::__createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> PoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(PoolSizes.size() * 1000);

    vk::checkError(vkCreateDescriptorPool(*m_pDevice, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CRenderPassGUI::__destroyDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(*m_pDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}
