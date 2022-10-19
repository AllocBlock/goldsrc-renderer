#include "PassShade.h"
#include "Function.h"
#include "RenderPassDescriptor.h"
#include "Gui.h"

void CRenderPassShade::setScene(CTempScene::Ptr vScene)
{
    m_pScene = vScene;
    m_ActorDataPositionSet.clear();
    m_pVertBuffer = vScene->generateVertexBuffer<CPipelineShade::SPointData>(m_AppInfo.pDevice, m_ActorDataPositionSet, m_VertexNum);
}

void CRenderPassShade::_initV()
{
}

SPortDescriptor CRenderPassShade::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    VkFormat DepthFormat = m_AppInfo.pDevice->getPhysicalDevice()->getBestDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::UNDEFINED });
    return Ports;
}

CRenderPassDescriptor CRenderPassShade::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
        m_pPortSet->getOutputPort("Depth"));
}

void CRenderPassShade::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassShade::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValues);

    // shade
    if (isNonEmptyAndValid(m_pVertBuffer))
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShade.bind(CommandBuffer, vImageIndex);

        _ASSERTE(m_pScene->getActorNum() == m_ActorDataPositionSet.size());
        for (size_t i = 0; i < m_ActorDataPositionSet.size(); ++i)
        {
            m_PipelineShade.updatePushConstant(CommandBuffer, m_pScene->getActor(i)->getTransform()->getAbsoluteModelMat4());
            vkCmdDraw(CommandBuffer, m_ActorDataPositionSet[i].Num, 1, m_ActorDataPositionSet[i].First, 0);
        }
    }

    end();
    return { CommandBuffer };
}

void CRenderPassShade::_destroyV()
{
    __destroyRecreateResources();
    destroyAndClear(m_pVertBuffer);

    IRenderPass::_destroyV();
}

void CRenderPassShade::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}
void CRenderPassShade::__createGraphicsPipelines()
{
    m_PipelineShade.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRenderPassShade::__createDepthResources()
{
    m_DepthImage.destroy();

    VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
    Function::createDepthImage(m_DepthImage, m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);

    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShade::__createFramebuffers()
{
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_AppInfo.ImageNum);

    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_DepthImage
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CRenderPassShade::__createRecreateResources()
{
    __createDepthResources();
    if (isValid())
    {
        __createGraphicsPipelines();
        m_PipelineShade.setImageNum(m_AppInfo.ImageNum);
        __createFramebuffers();
    }
}

void CRenderPassShade::__destroyRecreateResources()
{
    m_DepthImage.destroy();
    m_FramebufferSet.destroyAndClearAll();
    m_PipelineShade.destroy();
}

void CRenderPassShade::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    m_PipelineShade.updateUniformBuffer(vImageIndex, m_pCamera);
}