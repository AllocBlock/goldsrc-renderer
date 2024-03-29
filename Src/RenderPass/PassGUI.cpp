﻿#include "PassGUI.h"
#include "RenderPassDescriptor.h"
#include "InterfaceGui.h"

const std::string gChineseFont = "C:/windows/fonts/simhei.ttf";

void CRenderPassGUI::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();

    __createDescriptorPool();
    
    UI::init(m_pDevice, m_pWindow, m_DescriptorPool, m_ImageNum, get());
    CCommandBuffer::Ptr pCommandBuffer = m_Command.beginSingleTimeBuffer();
    UI::setFont(gChineseFont, pCommandBuffer);
    m_Command.endSingleTimeBuffer(pCommandBuffer);
}

void CRenderPassGUI::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
}

CRenderPassDescriptor CRenderPassGUI::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassGUI::_renderUIV()
{
}

void CRenderPassGUI::_destroyV()
{
    if (UI::isInitted())
        UI::destory();
    __destroyDescriptorPool();
    m_pWindow = nullptr;

    CRenderPassSingleFrameBuffer::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassGUI::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    _beginWithFramebuffer(vImageIndex);
    UI::draw(pCommandBuffer);
    _endWithFramebuffer();

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
