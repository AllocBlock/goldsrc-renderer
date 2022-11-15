#include "PassLine.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"

#include <vector>

void CLineRenderPass::setHighlightBoundingBox(SAABB vBoundingBox)
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
    CRenderPassSingle::_initV();

    __rerecordCommand();
}

SPortDescriptor CLineRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    Ports.addInput("Depth", { VK_FORMAT_D32_SFLOAT, {0, 0}, 1, EUsage::READ });
    return Ports;
}

CRenderPassDescriptor CLineRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getInputPort("Depth"));
}

void CLineRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    CRenderPassSingle::_onUpdateV(vUpdateState);

    VkExtent2D RefExtent = {0, 0};
    if (!_dumpReferenceExtentV(RefExtent)) return;

    if (vUpdateState.RenderpassUpdated || vUpdateState.InputImageUpdated || vUpdateState.ImageNum.IsUpdated || vUpdateState.ScreenExtent.IsUpdated)
    {
        if (isValid())
        {
            if (!vUpdateState.InputImageUpdated)
            {
                m_PipelineLine.create(m_pDevice, get(), RefExtent);
                m_PipelineLine.setImageNum(m_pAppInfo->getImageNum());
            }
        }

        __rerecordCommand();
    }
}

void CLineRenderPass::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pCamera);
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
    m_PipelineLine.destroy();

    CRenderPassSingle::_destroyV();
}

std::vector<VkCommandBuffer> CLineRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    bool RerecordCommand = false;
    if (m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        // init
        _beginWithFramebuffer(vImageIndex);
        m_PipelineLine.recordCommand(CommandBuffer, vImageIndex);
        _endWithFramebuffer();
    }
    return { CommandBuffer };
}

void CLineRenderPass::__rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}
