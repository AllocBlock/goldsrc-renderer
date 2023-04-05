#include "PassShade.h"
#include "ImageUtils.h"
#include "RenderPassDescriptor.h"
#include "ShadowMapDefines.h"

void CRenderPassShade::setShadowMapInfo(CCamera::CPtr vLightCamera)
{
    _ASSERTE(vLightCamera);
    m_pLightCamera = vLightCamera;
}

void CRenderPassShade::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);
    m_pCamera->setPos(glm::vec3(-3, 6, 4));
    m_pCamera->setTheta(120.0f);
    m_pCamera->setPhi(300.0f);
    
    __createRecreateResources();

    m_pPortSet->getInputPort("ShadowMap")->hookImageUpdate([this]
        {
            __updateShadowMapImages();
        }
    );
}

void CRenderPassShade::_initPortDescV(SPortDescriptor vDesc)
{
    SPortDescriptor Ports;
    Ports.addInput("ShadowMap", { gShadowMapImageFormat, {0, 0}, 1, EUsage::READ });
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
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
    if (!m_PipelineShade.isShadowMapReady())
        Log::log("Warning: shadow map image is not ready when shading");

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_FirstInputExtent, ClearValues);

    if (m_pVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShade.bind(CommandBuffer, vImageIndex);
        vkCmdDraw(CommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
    }
    _end();
    return { CommandBuffer };
}

void CRenderPassShade::_destroyV()
{
    __destroyRecreateResources();
    CRenderPassTempSceneBase::_destroyV();
}

void CRenderPassShade::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassShade::__createDepthResources()
{
    ImageUtils::createDepthImage(m_DepthImage, m_pDevice, m_FirstInputExtent);
    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShade::__createFramebuffers()
{
    uint32_t ImageNum = m_pAppInfo->getImageNum();
    m_FramebufferSet.init(ImageNum);
    
    for (uint32_t i = 0; i < ImageNum; ++i)
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
        __createFramebuffers();
        m_PipelineShade.create(m_pDevice, get(), m_FirstInputExtent);
        m_PipelineShade.setImageNum(m_pAppInfo->getImageNum());
        __updateShadowMapImages();
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
    _ASSERTE(m_pLightCamera);
    
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);

    m_PipelineShade.updateUniformBuffer(vImageIndex, m_pCamera, m_pLightCamera, gShadowMapSize);
}

void CRenderPassShade::__updateShadowMapImages()
{
    if (isValid() && m_PipelineShade.isValid())
    {
        std::vector<VkImageView> ShadowMapImageViewSet;
        for (uint32_t i = 0; i < m_pAppInfo->getImageNum(); ++i)
        {
            ShadowMapImageViewSet.emplace_back(m_pPortSet->getInputPort("ShadowMap")->getImageV(i));
        }

        m_PipelineShade.setShadowMapImageViews(ShadowMapImageViewSet);
    }
}
