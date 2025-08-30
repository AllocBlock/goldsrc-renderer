#include "PassVisualize.h"
#include "RenderPassDescriptor.h"
#include "ImageUtils.h"

#include <vector>


void CRenderPassVisualize::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{
    m_PipelineSet.Triangle.add(vTriangle, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addTriangle(const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC, const glm::vec3& vColor)
{
    addTriangle(Visualize::Triangle{ vA, vB, vC }, vColor);
}

void CRenderPassVisualize::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{
    m_PipelineSet.Line.add(vLine, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addLine(const glm::vec3& vStart, const glm::vec3& vEnd, const glm::vec3& vColor)
{
    addLine(Visualize::Line{ vStart, vEnd }, vColor);
}

void CRenderPassVisualize::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{
    m_PipelineSet.Point.add(vPoint, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_PipelineSet.Primitive3D.add(E3DPrimitiveType::SPHERE, vCenter, vScale, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_PipelineSet.Primitive3D.add(E3DPrimitiveType::CUBE, vCenter, vScale, vColor);
    __rerecordCommand();
}

void CRenderPassVisualize::clearAll()
{
    m_PipelineSet.Triangle.clear();
    m_PipelineSet.Line.clear();
    m_PipelineSet.Point.clear();
    m_PipelineSet.Primitive3D.clear();
    __rerecordCommand();
}

void CRenderPassVisualize::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();

    m_PipelineSet.Triangle.create(m_pDevice, get(), m_ScreenExtent);
    m_PipelineSet.Line.create(m_pDevice, get(), m_ScreenExtent);
    m_PipelineSet.Point.create(m_pDevice, get(), m_ScreenExtent);
    m_PipelineSet.Primitive3D.create(m_pDevice, get(), m_ScreenExtent);

    __rerecordCommand();
}

void CRenderPassVisualize::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    vioDesc.addInput("Depth", { VK_FORMAT_D32_SFLOAT, {0, 0}, 1, EImageUsage::DEPTH_ATTACHMENT });
}

CRenderPassDescriptor CRenderPassVisualize::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getInputPort("Depth"));
}

void CRenderPassVisualize::_updateV()
{
    _ASSERTE(m_pSceneInfo);
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_PipelineSet.Triangle.updateUniformBuffer(pCamera);
    m_PipelineSet.Line.updateUniformBuffer(pCamera);
    m_PipelineSet.Point.updateUniformBuffer(pCamera);
    m_PipelineSet.Primitive3D.updateUniformBuffer(pCamera);
}

void CRenderPassVisualize::_destroyV()
{
    m_PipelineSet.Triangle.destroy();
    m_PipelineSet.Line.destroy();
    m_PipelineSet.Point.destroy();
    m_PipelineSet.Primitive3D.destroy();

    CRenderPassSingleFrameBuffer::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassVisualize::_requestCommandBuffersV()
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();

    if (m_RerecordCommand)
    {
        // init
        _beginWithFramebuffer();
        m_PipelineSet.Triangle.recordCommandV(pCommandBuffer);
        m_PipelineSet.Line.recordCommandV(pCommandBuffer);
        m_PipelineSet.Point.recordCommandV(pCommandBuffer);
        m_PipelineSet.Primitive3D.recordCommand(pCommandBuffer);
        _endWithFramebuffer();
        m_RerecordCommand = false;
    }
    return { pCommandBuffer->get() };
}

void CRenderPassVisualize::__rerecordCommand()
{
    m_RerecordCommand = true;
}
