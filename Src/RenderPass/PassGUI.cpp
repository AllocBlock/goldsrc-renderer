#include "PassGUI.h"
#include "Common.h"
#include "Log.h"
#include "AppInfo.h"
#include "RenderPassDescriptor.h"
#include "InterfaceUI.h"

const std::string gChineseFont = "C:/windows/fonts/simhei.ttf";

void CRenderPassGUI::_initV()
{
    IRenderPass::_initV();

    _ASSERTE(m_pWindow);
    
    __createDescriptorPool();
}

SPortDescriptor CRenderPassGUI::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    return Ports;
}

CRenderPassDescriptor CRenderPassGUI::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassGUI::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.InputImageUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        VkExtent2D RefExtent = { 0, 0 };
        bool HasExtent = _dumpInputPortExtent("Main", RefExtent);

        if (HasExtent && isValid())
            __createFramebuffer(RefExtent);

        if (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated)
        {
            if (isValid())
            {
                UI::init(m_pDevice, m_pWindow, m_DescriptorPool, m_pAppInfo->getImageNum(), get());
                VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
                UI::setFont(gChineseFont, CommandBuffer);
                m_Command.endSingleTimeBuffer(CommandBuffer);
            }
        }
    }
}

void CRenderPassGUI::_renderUIV()
{
    UI::text(u8"默认GUI");
}

void CRenderPassGUI::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    UI::destory();
    __destroyDescriptorPool();
    m_pWindow = nullptr;
}

std::vector<VkCommandBuffer> CRenderPassGUI::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    begin(CommandBuffer, m_FramebufferSet[vImageIndex], { ClearValue });
    UI::draw(CommandBuffer);
    end();

    return { CommandBuffer };
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

void CRenderPassGUI::__createFramebuffer(VkExtent2D vExtent)
{
    if (!isValid()) return;
    if (!m_pPortSet->isImageReady()) return;

    m_FramebufferSet.destroyAndClearAll();

    uint32_t ImageNum = m_pAppInfo->getImageNum();

    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
        };

        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
}