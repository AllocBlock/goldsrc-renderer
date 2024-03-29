#include "PassSprite.h"
#include "SceneCommon.h"
#include "RenderPassDescriptor.h"

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


void CRenderPassSprite::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
}

CRenderPassDescriptor CRenderPassSprite::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassSprite::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();
    
    m_PipelineSprite.create(m_pDevice, get(), m_ScreenExtent, m_ImageNum);
}

void CRenderPassSprite::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    _beginWithFramebuffer(vImageIndex);
    m_PipelineSprite.recordCommand(pCommandBuffer, vImageIndex);
    _endWithFramebuffer();
    return { pCommandBuffer->get() };
}

void CRenderPassSprite::_destroyV()
{
    m_PipelineSprite.destroy();

    CRenderPassSingleFrameBuffer::_destroyV();
}

bool CRenderPassSprite::_dumpReferenceExtentV(VkExtent2D& voExtent)
{
    voExtent = m_ScreenExtent;
    return true;
}

std::vector<VkImageView> CRenderPassSprite::_getAttachmentsV(uint32_t vIndex)
{
    return
    {
        m_pPortSet->getOutputPort("Main")->getImageV(vIndex)
    };
}

std::vector<VkClearValue> CRenderPassSprite::_getClearValuesV()
{
    return DefaultClearValueColor;
}

void CRenderPassSprite::__updateUniformBuffer(uint32_t vImageIndex)
{
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    m_PipelineSprite.updateUniformBuffer(vImageIndex, pCamera);
}
