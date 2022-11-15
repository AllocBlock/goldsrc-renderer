#include "PassSprite.h"
#include "SceneCommon.h"
#include "RenderPassDescriptor.h"
#include "Function.h"

void CRenderPassSprite::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);
    m_pCamera->setPos(glm::vec3(1.0, 0.0, 0.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));
    
    __createRecreateResources();
}

SPortDescriptor CRenderPassSprite::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    return Ports;
}

CRenderPassDescriptor CRenderPassSprite::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassSprite::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_FirstInputExtent, ClearValueSet);
    m_Pipeline.recordCommand(CommandBuffer, vImageIndex);
    _end();
    return { CommandBuffer };
}

void CRenderPassSprite::_destroyV()
{
    __destroyRecreateResources();

    IRenderPass::_destroyV();
}

void CRenderPassSprite::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassSprite::__createGraphicsPipeline()
{
    m_Pipeline.create(m_pDevice, get(), m_FirstInputExtent);
}

void CRenderPassSprite::__createFramebuffers()
{
    _ASSERTE(isValid());

    uint32_t ImageNum = m_pAppInfo->getImageNum();
    m_FramebufferSet.init(ImageNum);
    for (uint32_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> Attachments =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
        };
        
        m_FramebufferSet[i]->create(m_pDevice, get(), Attachments, m_FirstInputExtent);
    }
}

void CRenderPassSprite::__createRecreateResources()
{
    if (isValid())
    {
        __createFramebuffers();
        __createGraphicsPipeline();
        m_Pipeline.setImageNum(m_pAppInfo->getImageNum());

        const std::vector<SGoldSrcSprite> SpriteSet =
        {
            {
                glm::vec3(0.0, 0.0, 0.0),
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                EGoldSrcSpriteType::PARALLEL,
                Scene::generateBlackPurpleGrid(16, 16, 4),
            },
            {
                glm::vec3(0.0, 3.0, 0.0),
                glm::vec3(0.0, 0.0, 0.0),
                0.5,
                EGoldSrcSpriteType::PARALLEL,
                Scene::generateDiagonalGradientGrid(64, 64, 255, 0, 0, 0, 255, 0),
            },
            {
                glm::vec3(0.0, 6.0, 0.0),
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                EGoldSrcSpriteType::PARALLEL_UP_RIGHT,
                Scene::generateDiagonalGradientGrid(64, 64, 255, 255, 0, 0, 255, 0),
            },
            {
                glm::vec3(0.0, 9.0, 0.0),
                glm::vec3(-90.0, 0.0, 0.0),
                1.0,
                EGoldSrcSpriteType::PARALLEL_ORIENTED,
                Scene::generateDiagonalGradientGrid(64, 64, 255, 255, 0, 0, 255, 255),
            },
            {
                glm::vec3(0.0, 12.0, 0.0),
                glm::vec3(-90.0, 0.0, 0.0),
                1.0,
                EGoldSrcSpriteType::ORIENTED,
                Scene::generateDiagonalGradientGrid(64, 64, 0, 255, 255, 0, 0, 255),
            },
            {
                glm::vec3(0.0, 15.0, 0.0),
                glm::vec3(0.0, 0.0, 0.0),
                1.0,
                EGoldSrcSpriteType::FACING_UP_RIGHT,
                Scene::generateDiagonalGradientGrid(64, 64, 0, 255, 0, 255, 0, 255),
            }
        };

        m_Pipeline.setSprites(SpriteSet);
    }
}

void CRenderPassSprite::__destroyRecreateResources()
{
    m_FramebufferSet.destroyAndClearAll();
    m_Pipeline.destroy();
}

void CRenderPassSprite::__updateUniformBuffer(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);

    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 EyeDirection = m_pCamera->getFront();

    m_Pipeline.updateUniformBuffer(vImageIndex, View, Proj, EyePos, EyeDirection);
}
