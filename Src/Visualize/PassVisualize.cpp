#include "PassVisualize.h"
#include "RenderPassDescriptor.h"
#include "Function.h"

#include <vector>


void CRenderPassVisualize::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{
    m_PipelineSet.Triangle.get().add(vTriangle, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{
    m_PipelineSet.Line.get().add(vLine, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{
    m_PipelineSet.Point.get().add(vPoint, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::clearAll()
{
    m_PipelineSet.Triangle.get().clear();
    m_PipelineSet.Line.get().clear();
    m_PipelineSet.Point.get().clear();
    __rerecordCommand();
}

void CRenderPassVisualize::_initV()
{
    CRenderPassSingle::_initV();

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();
    
    m_PipelineSet.Triangle.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());
    m_PipelineSet.Line.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());
    m_PipelineSet.Point.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());

    __rerecordCommand();
}

void CRenderPassVisualize::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    vioDesc.addInput("Depth", { VK_FORMAT_D32_SFLOAT, {0, 0}, 1, EUsage::WRITE });
}

CRenderPassDescriptor CRenderPassVisualize::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getInputPort("Depth"));
}

void CRenderPassVisualize::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_PipelineSet.Triangle.updateV(vUpdateState);
    m_PipelineSet.Line.updateV(vUpdateState);
    m_PipelineSet.Point.updateV(vUpdateState);
    
    CRenderPassSingle::_onUpdateV(vUpdateState);
}

void CRenderPassVisualize::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pCamera);
    m_PipelineSet.Triangle.get().updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineSet.Line.get().updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineSet.Point.get().updateUniformBuffer(vImageIndex, m_pCamera);
}

void CRenderPassVisualize::_destroyV()
{
    m_PipelineSet.Triangle.destroy();
    m_PipelineSet.Line.destroy();
    m_PipelineSet.Point.destroy();

    CRenderPassSingle::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassVisualize::_requestCommandBuffersV(uint32_t vImageIndex)
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
        m_PipelineSet.Triangle.get().recordCommandV(CommandBuffer, vImageIndex);
        m_PipelineSet.Line.get().recordCommandV(CommandBuffer, vImageIndex);
        m_PipelineSet.Point.get().recordCommandV(CommandBuffer, vImageIndex);
        _endWithFramebuffer();
    }
    return { CommandBuffer };
}

void CRenderPassVisualize::__rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}
