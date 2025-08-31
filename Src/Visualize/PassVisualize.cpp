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

CPortSet::Ptr CRenderPassVisualize::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addOutput("Main", { VK_FORMAT_B8G8R8A8_UNORM, {0, 0}, 0, EImageUsage::COLOR_ATTACHMENT });
    PortDesc.addOutput("Depth", { VK_FORMAT_D24_UNORM_S8_UINT, {0, 0}, 0, EImageUsage::DEPTH_ATTACHMENT });
    return make<CPortSet>(PortDesc);
}

void CRenderPassVisualize::_initV()
{
    m_pMainImage = make<vk::CImage>();
    VkFormat MainFormat = m_pPortSet->getOutputPortInfo("Main").Format;
    ImageUtils::createImage2d(*m_pMainImage, m_pDevice, m_ScreenExtent, MainFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    m_pPortSet->setOutput("Main", m_pMainImage);

    m_pDepthImage = make<vk::CImage>();
    VkFormat DepthFormat = m_pPortSet->getOutputPortInfo("Depth").Format;
    ImageUtils::createDepthImage(*m_pDepthImage, m_pDevice, m_ScreenExtent, VK_IMAGE_USAGE_SAMPLED_BIT, DepthFormat);
    m_pPortSet->setOutput("Depth", m_pDepthImage);

    m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    m_RenderInfoDescriptor.setDepthAttachment(m_pPortSet->getOutputPort("Depth"));

    m_PipelineSet.Triangle.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);
    m_PipelineSet.Line.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);
    m_PipelineSet.Point.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);
    m_PipelineSet.Primitive3D.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);

    __rerecordCommand();
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
    destroyAndClear(m_pMainImage);
    destroyAndClear(m_pDepthImage);

    m_PipelineSet.Triangle.destroy();
    m_PipelineSet.Line.destroy();
    m_PipelineSet.Point.destroy();
    m_PipelineSet.Primitive3D.destroy();
}

std::vector<VkCommandBuffer> CRenderPassVisualize::_requestCommandBuffersV()
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();

    //if (m_RerecordCommand)
    if (true)
    {
        // init
        _beginCommand(pCommandBuffer);
        _beginRendering(pCommandBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent));
        m_PipelineSet.Triangle.recordCommandV(pCommandBuffer);
        m_PipelineSet.Line.recordCommandV(pCommandBuffer);
        m_PipelineSet.Point.recordCommandV(pCommandBuffer);
        m_PipelineSet.Primitive3D.recordCommand(pCommandBuffer);
        _endRendering();
        _endCommand();
        m_RerecordCommand = false;
    }
    return { pCommandBuffer->get() };
}

void CRenderPassVisualize::__rerecordCommand()
{
    m_RerecordCommand = true;
}
