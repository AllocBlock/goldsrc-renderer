﻿#include "GUIPass.h"
#include "Common.h"
#include "Log.h"
#include "AppInfo.h"
#include "RenderPassDescriptor.h"
#include "Gui.h"

const std::string gChineseFont = "C:/windows/fonts/simhei.ttf";

void CGUIRenderPass::_initV()
{
    IRenderPass::_initV();

    _ASSERTE(m_pWindow);

    uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.ImageNum);
    __createDescriptorPool();
}

SPortDescriptor CGUIRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main");
    return Ports;
}

CRenderPassDescriptor CGUIRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CGUIRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.CommandUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        if (isValid())
        {
            uint32_t NumImage = static_cast<uint32_t>(m_AppInfo.ImageNum);
            UI::init(m_AppInfo, m_pWindow, m_DescriptorPool, NumImage, get());
            VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
            UI::addFont(gChineseFont, CommandBuffer);
            m_Command.endSingleTimeBuffer(CommandBuffer);
        }
    }

    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createFramebuffer();
    }
}

void CGUIRenderPass::_renderUIV()
{
    UI::text(u8"默认GUI");
}

void CGUIRenderPass::_destroyV()
{
    if (*m_AppInfo.pDevice == VK_NULL_HANDLE) return;

    m_FramebufferSet.destroyAndClearAll();

    UI::destory();

    __destroyDescriptorPool();

    m_pWindow = nullptr;

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CGUIRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, { ClearValue });
    UI::draw(CommandBuffer);
    end();

    return { CommandBuffer };
}

void CGUIRenderPass::__createDescriptorPool()
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

    vk::checkError(vkCreateDescriptorPool(*m_AppInfo.pDevice, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CGUIRenderPass::__destroyDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(*m_AppInfo.pDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void CGUIRenderPass::__createFramebuffer()
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    uint32_t ImageNum = static_cast<uint32_t>(m_AppInfo.ImageNum);

    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}