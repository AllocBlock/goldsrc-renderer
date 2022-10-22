#include "PassShade.h"
#include "Function.h"
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
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
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

SPortDescriptor CRenderPassShade::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInput("ShadowMap", { gShadowMapImageFormat, {0, 0}, 1, EUsage::READ });
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
    if (!m_PipelineShade.isShadowMapReady())
        Log::log("Warning: shadow map image is not ready when shading");

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValues(2);
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValues[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValues);

    if (m_pVertBuffer->isValid())
    {
        VkDeviceSize Offsets[] = { 0 };
        VkBuffer VertBuffer = *m_pVertBuffer;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_PipelineShade.bind(CommandBuffer, vImageIndex);
        vkCmdDraw(CommandBuffer, m_VertexNum, 1, 0, 0);
    }
    end();
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
    Function::createDepthImage(m_DepthImage, m_AppInfo.pDevice, m_AppInfo.Extent);
    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassShade::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.init(ImageNum);
    
    for (size_t i = 0; i < ImageNum; ++i)
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
        __createFramebuffers();
        m_PipelineShade.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
        m_PipelineShade.setImageNum(m_AppInfo.ImageNum);
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

    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    m_PipelineShade.updateUniformBuffer(vImageIndex, m_pCamera, m_pLightCamera, gShadowMapSize);
}

void CRenderPassShade::__updateShadowMapImages()
{
    if (isValid() && m_PipelineShade.isValid())
    {
        std::vector<VkImageView> ShadowMapImageViewSet;
        for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
        {
            ShadowMapImageViewSet.emplace_back(m_pPortSet->getInputPort("ShadowMap")->getImageV(i));
        }

        m_PipelineShade.setShadowMapImageViews(ShadowMapImageViewSet);
    }
}
