#include "PassVisPhysics.h"
#include "RenderPassDescriptor.h"
#include "InterfaceUI.h"

void CRenderPassVisPhysics::setPhysicsEngine(CPhysicsEngine::Ptr vEngine)
{
    m_pEngine = vEngine;
    m_pEngine->addCollisionHook([this](glm::vec3 vPos, glm::vec3 vNormal)
        {
            m_PipelineVisCollidePoint.addCollidePoint(vPos, vNormal);
        });
}

void CRenderPassVisPhysics::_initV()
{
}

SPortDescriptor CRenderPassVisPhysics::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    return Ports;
}

CRenderPassDescriptor CRenderPassVisPhysics::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassVisPhysics::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassVisPhysics::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (!m_pEngine || !m_pCamera) return {};

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    VkClearValue ClearValue;
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, { ClearValue });

    // visualize collider
    if (m_ShowCollider)
    {
        m_PipelineVisCollider.startRecord(CommandBuffer, vImageIndex); // TIPS: contain bind
        for (size_t i = 0; i < m_pEngine->getRigidBodyNum(); ++i)
        {
            auto pRigidBody = m_pEngine->getRigidBody(i);
            m_PipelineVisCollider.drawCollider(pRigidBody->pCollider);
        }
        m_PipelineVisCollider.endRecord();
    }

    // visualize collide point
    m_PipelineVisCollidePoint.record(CommandBuffer, vImageIndex, m_pCamera->getPos());

    end();
    return { CommandBuffer };
}

void CRenderPassVisPhysics::_destroyV()
{
    __destroyRecreateResources();

    IRenderPass::_destroyV();
}

void CRenderPassVisPhysics::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassVisPhysics::_renderUIV()
{
    UI::toggle(u8"��ʾ��ײ��", m_ShowCollider);
}

void CRenderPassVisPhysics::__createGraphicsPipelines()
{
    m_PipelineVisCollider.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_PipelineVisCollidePoint.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRenderPassVisPhysics::__createFramebuffers()
{
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_AppInfo.ImageNum);

    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i)
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CRenderPassVisPhysics::__createRecreateResources()
{
    if (isValid())
    {
        __createGraphicsPipelines();
        m_PipelineVisCollider.setImageNum(m_AppInfo.ImageNum);
        m_PipelineVisCollidePoint.setImageNum(m_AppInfo.ImageNum);
        __createFramebuffers();
    }
}

void CRenderPassVisPhysics::__destroyRecreateResources()
{
    m_FramebufferSet.destroyAndClearAll();
    m_PipelineVisCollider.destroy();
    m_PipelineVisCollidePoint.destroy();
}

void CRenderPassVisPhysics::__updateUniformBuffer(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_AppInfo.Extent.width, m_AppInfo.Extent.height);
    
    m_PipelineVisCollider.updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineVisCollidePoint.updateUniformBuffer(vImageIndex, m_pCamera);
}
