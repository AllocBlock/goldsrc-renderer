#include "PassVisualize.h"
#include "RenderPassDescriptor.h"
#include "ImageUtils.h"

#include <vector>


void CRenderPassVisualize::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{
    m_PipelineSet.Triangle.get().add(vTriangle, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addTriangle(const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC, const glm::vec3& vColor)
{
    addTriangle(Visualize::Triangle{ vA, vB, vC }, vColor);
}

void CRenderPassVisualize::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{
    m_PipelineSet.Line.get().add(vLine, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addLine(const glm::vec3& vStart, const glm::vec3& vEnd, const glm::vec3& vColor)
{
    addLine(Visualize::Line{ vStart, vEnd }, vColor);
}

void CRenderPassVisualize::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{
    m_PipelineSet.Point.get().add(vPoint, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_PipelineSet.Primitive3D.get().add(E3DPrimitiveType::SPHERE, vCenter, vScale, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_PipelineSet.Primitive3D.get().add(E3DPrimitiveType::CUBE, vCenter, vScale, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::clearAll()
{
    m_PipelineSet.Triangle.get().clear();
    m_PipelineSet.Line.get().clear();
    m_PipelineSet.Point.get().clear();
    m_PipelineSet.Primitive3D.get().clear();
    __rerecordCommand();
}

void CRenderPassVisualize::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();
    
    m_PipelineSet.Triangle.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());
    m_PipelineSet.Line.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());
    m_PipelineSet.Point.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());
    m_PipelineSet.Primitive3D.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum());

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
    if (vUpdateState.ImageNum.IsUpdated || vUpdateState.RenderpassUpdated)
    {
        __rerecordCommand();
    }

    m_PipelineSet.Triangle.updateV(vUpdateState);
    m_PipelineSet.Line.updateV(vUpdateState);
    m_PipelineSet.Point.updateV(vUpdateState);
    m_PipelineSet.Primitive3D.updateV(vUpdateState);
    
    CRenderPassSingleFrameBuffer::_onUpdateV(vUpdateState);
}

void CRenderPassVisualize::_updateV(uint32_t vImageIndex)
{
    _ASSERTE(m_pSceneInfo);
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_PipelineSet.Triangle.get().updateUniformBuffer(vImageIndex, pCamera);
    m_PipelineSet.Line.get().updateUniformBuffer(vImageIndex, pCamera);
    m_PipelineSet.Point.get().updateUniformBuffer(vImageIndex, pCamera);
    m_PipelineSet.Primitive3D.get().updateUniformBuffer(vImageIndex, pCamera);
}

void CRenderPassVisualize::_destroyV()
{
    m_PipelineSet.Triangle.destroy();
    m_PipelineSet.Line.destroy();
    m_PipelineSet.Point.destroy();
    m_PipelineSet.Primitive3D.destroy();

    CRenderPassSingleFrameBuffer::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassVisualize::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    bool RerecordCommand = false;
    if (m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        --m_RerecordCommandTimes;
    }
    if (true)
    {
        // init
        _beginWithFramebuffer(vImageIndex);
        m_PipelineSet.Triangle.get().recordCommandV(pCommandBuffer, vImageIndex);
        m_PipelineSet.Line.get().recordCommandV(pCommandBuffer, vImageIndex);
        m_PipelineSet.Point.get().recordCommandV(pCommandBuffer, vImageIndex);
        m_PipelineSet.Primitive3D.get().recordCommand(pCommandBuffer, vImageIndex);
        _endWithFramebuffer();
    }
    return { pCommandBuffer->get() };
}

void CRenderPassVisualize::__rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}
