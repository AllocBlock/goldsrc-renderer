#include "PassShade.h"
#include "ImageUtils.h"
#include "RenderPassDescriptor.h"
#include "..\..\Gui\InterfaceGui.h"

void CRenderPassShade::setScene(CScene<CMeshData>::Ptr vScene)
{
    m_pScene = vScene;
    m_pVertBuffer = vScene->generateVertexBuffer<CPipelineShade::SPointData>(m_pDevice);
}

void CRenderPassShade::_initV()
{
}

void CRenderPassShade::_initPortDescV(SPortDescriptor vDesc)
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    VkFormat DepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    Ports.addOutput("Depth", { DepthFormat, {0, 0}, 1, EImageUsage::DEPTH_ATTACHMENT });
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

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_FirstInputExtent, ClearValues);

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

    _end();
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
    m_PipelineShade.create(m_pDevice, get(), m_FirstInputExtent);
}

void CRenderPassShade::__createDepthResources()
{
    m_DepthImage.destroy();

    VkFormat DepthFormat = m_pPortSet->getOutputPortInfo("Depth").Format;
    ImageUtils::createDepthImage(m_DepthImage, m_pDevice, m_FirstInputExtent, NULL, DepthFormat);

    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShade::__createFramebuffers()
{
    m_FramebufferSet.destroyAndClearAll();
    m_FramebufferSet.init(m_pAppInfo->getImageNum());

    for (uint32_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_DepthImage
        };

        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, m_FirstInputExtent);
    }
}

void CRenderPassShade::__createRecreateResources()
{
    __createDepthResources();
    if (isValid())
    {
        __createGraphicsPipelines();
        m_PipelineShade.setImageNum(m_pAppInfo->getImageNum());
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
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);

    m_PipelineShade.updateUniformBuffer(vImageIndex, m_pCamera);
}