#include "PassSprite.h"
#include "SceneCommon.h"
#include "RenderPassDescriptor.h"
#include "Function.h"

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
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
}

CRenderPassDescriptor CRenderPassSprite::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassSprite::_initV()
{
    CRenderPassSingle::_initV();

    m_PipelineSpriteCreator.init(m_pDevice, weak_from_this(), m_pAppInfo->getScreenExtent(), true, m_pAppInfo->getImageNum(),
        [this](CPipelineSprite& vPipeline) { vPipeline.setSprites(m_SpriteSet); }
    );
}

void CRenderPassSprite::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    _beginWithFramebuffer(vImageIndex);
    m_PipelineSpriteCreator.get().recordCommand(CommandBuffer, vImageIndex);
    _endWithFramebuffer();
    return { CommandBuffer };
}

void CRenderPassSprite::_destroyV()
{
    m_PipelineSpriteCreator.destroy();

    CRenderPassSingle::_destroyV();
}

void CRenderPassSprite::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_PipelineSpriteCreator.updateV(vUpdateState);

    CRenderPassSingle::_onUpdateV(vUpdateState);
}

bool CRenderPassSprite::_dumpReferenceExtentV(VkExtent2D& voExtent)
{
    voExtent = m_pAppInfo->getScreenExtent();
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
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 EyeDirection = m_pCamera->getFront();

    m_PipelineSpriteCreator.get().updateUniformBuffer(vImageIndex, View, Proj, EyePos, EyeDirection);
}
