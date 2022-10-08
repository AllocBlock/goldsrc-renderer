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

void CLineRenderPass::setHighlightBoundingBox(Math::S3DBoundingBox vBoundingBox)
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

    __rerecordCommand();
}

SPortDescriptor CLineRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    Ports.addInput("Depth", { VK_FORMAT_D32_SFLOAT, m_AppInfo.Extent, 1, EUsage::READ });
    return Ports;
}

CRenderPassDescriptor CLineRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getInputPort("Depth"));
}

void CLineRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createFramebuffers();
        if (isValid())
        {
            m_PipelineLine.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
            m_PipelineLine.setImageNum(m_AppInfo.ImageNum);
        }

        __rerecordCommand();
    }
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
    m_FramebufferSet.destroyAndClearAll();

    m_PipelineLine.destroy();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CLineRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

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

void CLineRenderPass::__createFramebuffers()
{
    if (!isValid()) return;

    m_AppInfo.pDevice->waitUntilIdle();
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_pPortSet->getInputPort("Depth")->getImageV(),
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}