#include "PassSprite.h"
#include "SceneCommon.h"

CRenderPassSprite::CRenderPassSprite()
{
    m_SpriteSet =
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
}

sptr<CPortSet> CRenderPassSprite::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    return make<CPortSet>(PortDesc);
}

void CRenderPassSprite::_initV()
{
    m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    
    m_PipelineSprite.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);
}

void CRenderPassSprite::_updateV()
{
    __updateUniformBuffer();
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV()
{
    sptr<CCommandBuffer> pCommandBuffer = _getCommandBuffer();

    _beginCommand(pCommandBuffer);
    _beginRendering(pCommandBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent));
    m_PipelineSprite.recordCommand(pCommandBuffer);
    _endCommand();
    _endRendering();
    return { pCommandBuffer->get() };
}

void CRenderPassSprite::_destroyV()
{
    m_PipelineSprite.destroy();
}

void CRenderPassSprite::__updateUniformBuffer()
{
    sptr<CCamera> pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_PipelineSprite.updateUniformBuffer(pCamera);
}
