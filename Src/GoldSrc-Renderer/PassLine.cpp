#include "PassLine.h"
#include "Common.h"
#include "Descriptor.h"
#include "Function.h"
#include "Gui.h"
#include "RenderPassDescriptor.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

void CLineRenderPass::setHighlightBoundingBox(S3DBoundingBox vBoundingBox)
{
    auto pObject = make<SGuiObject>();

    std::array<glm::vec3, 8> Vertices =
    {
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Min.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Min.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Max.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Max.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Min.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Min.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Max.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Max.y, vBoundingBox.Max.z),
    };

    pObject->Data =
    {
        Vertices[0], Vertices[1],  Vertices[1], Vertices[2],  Vertices[2], Vertices[3],  Vertices[3], Vertices[0],
        Vertices[4], Vertices[5],  Vertices[5], Vertices[6],  Vertices[6], Vertices[7],  Vertices[7], Vertices[4],
        Vertices[0], Vertices[4],  Vertices[1], Vertices[5],  Vertices[2], Vertices[6],  Vertices[3], Vertices[7]
    };

    m_PipelineLine.setObject("HighlightBoundingBox", std::move(pObject));
    __rerecordCommand();
}

void CLineRenderPass::removeHighlightBoundingBox()
{
    m_PipelineLine.removeObject("HighlightBoundingBox");
    __rerecordCommand();
}

void CLineRenderPass::addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd)
{
    auto pObject = make<SGuiObject>();
    pObject->Data = { vStart, vEnd };
    m_PipelineLine.setObject(vName, std::move(pObject));
    __rerecordCommand();
}

void CLineRenderPass::_initV()
{
    IRenderPass::_initV();

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();
    m_pPortSet->getOutputPort("Output")->hookUpdate([=] { m_NeedUpdateFramebuffer = true; });

    __rerecordCommand();
}

SPortDescriptor CLineRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInput("Input", { m_AppInfo.ImageFormat, m_AppInfo.Extent, m_AppInfo.ImageNum });
    Ports.addInput("Depth", { VK_FORMAT_D32_SFLOAT, m_AppInfo.Extent, 1 });
    Ports.addOutput("Output", { m_AppInfo.ImageFormat, m_AppInfo.Extent, m_AppInfo.ImageNum });
    return Ports;
}

void CLineRenderPass::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    __rerecordCommand();
}

void CLineRenderPass::_updateV(uint32_t vImageIndex)
{
    m_PipelineLine.updateUniformBuffer(vImageIndex, m_pCamera);
}

void CLineRenderPass::_renderUIV()
{
    if (UI::collapse(u8"³¡¾°UI"))
    {
    }
}

void CLineRenderPass::_destroyV()
{
    __destroyRecreateResources();

    m_Command.clear();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CLineRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_NeedUpdateFramebuffer)
    {
        __createFramebuffers();
        __rerecordCommand();
    }

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    bool RerecordCommand = false;
    if (m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        // init
        std::vector<VkClearValue> ClearValueSet(2);
        ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValueSet[1].depthStencil = { 1.0f, 0 };

        begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);
        m_PipelineLine.recordCommand(CommandBuffer, vImageIndex);
        end();
    }
    return { CommandBuffer };
}

void CLineRenderPass::__rerecordCommand()
{
    m_RerecordCommandTimes += m_AppInfo.ImageNum;
}

void CLineRenderPass::__createRecreateResources()
{
    m_PipelineLine.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_PipelineLine.setImageNum(m_AppInfo.ImageNum);
}

void CLineRenderPass::__destroyRecreateResources()
{
    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    m_PipelineLine.destroy();
}

void CLineRenderPass::__createRenderPass()
{
    auto Info = CRenderPassDescriptor::generateSingleSubpassInfo(m_RenderPassPosBitField, m_AppInfo.ImageFormat, VK_FORMAT_D32_SFLOAT);
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
}

void CLineRenderPass::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_CommandName, static_cast<uint32_t>(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
}

void CLineRenderPass::__createFramebuffers()
{
    m_AppInfo.pDevice->waitUntilIdle();
    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.resize(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Output")->getImageV(i),
            m_pPortSet->getInputPort("Depth")->getImageV(),
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}