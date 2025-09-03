#include "PassGoldSrc.h"
#include "Common.h"
#include "ImageUtils.h"
#include "InterfaceGui.h"
#include "Log.h"
#include "ComponentIconRenderer.h"

#include <vector>
#include <set>
#include <fstream>

void CRenderPassGoldSrc::_onSceneInfoSet(ptr<SSceneInfo> vScene)
{
    m_pDevice->waitUntilIdle();
    m_CurTextureIndex = 0;

    __createVertexBuffer();
    __destroySceneResources();
    __createSceneResources();
    __updateTextureView();
}

void CRenderPassGoldSrc::rerecordAllCommand()
{
    m_pRerecord->requestRecordForAll();
}

CPortSet::Ptr CRenderPassGoldSrc::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addOutput("Main", { VK_FORMAT_B8G8R8A8_UNORM, {0, 0}, 0, EImageUsage::COLOR_ATTACHMENT });
    PortDesc.addOutput("Depth", { VK_FORMAT_D24_UNORM_S8_UINT, {0, 0}, 0, EImageUsage::DEPTH_ATTACHMENT });
    return make<CPortSet>(PortDesc);
}

void CRenderPassGoldSrc::_initV()
{
    m_pRerecord = make<CRerecordState>();
    for (const auto& Name : _getSecondaryCommandBufferNamesV())
        m_pRerecord->addField(Name);
    
    VkExtent2D RefExtent = m_ScreenExtent;

    m_pMainImage = make<vk::CImage>();
    VkFormat MainFormat = m_pPortSet->getOutputPortInfo("Main").Format;
    ImageUtils::createImage2d(*m_pMainImage, m_pDevice, m_ScreenExtent, MainFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    m_pPortSet->setOutput("Main", m_pMainImage);
    m_pMainImage->setDebugName("PassGoldSrc-Main");

    m_pDepthImage = make<vk::CImage>();
    VkFormat DepthFormat = m_pPortSet->getOutputPortInfo("Depth").Format;
    ImageUtils::createDepthImage(*m_pDepthImage, m_pDevice, RefExtent, VK_IMAGE_USAGE_SAMPLED_BIT, DepthFormat);
    m_pPortSet->setOutput("Depth", m_pDepthImage);
    m_pMainImage->setDebugName("PassGoldSrc-Depth");

    m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    m_RenderInfoDescriptor.setDepthAttachment(m_pPortSet->getOutputPort("Depth"));

    m_PipelineSet.Normal.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.BlendTextureAlpha.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.BlendAlphaTest.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.BlendAdditive.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.Simple.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.Sky.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.Sprite.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.Icon.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    m_PipelineSet.Text.create(m_pDevice, m_RenderInfoDescriptor, RefExtent);
    
    __createSceneResources();

    rerecordAllCommand();
}

void CRenderPassGoldSrc::_updateV()
{
    __updateAllUniformBuffer();
}

void CRenderPassGoldSrc::_renderUIV()
{
    if (UI::collapse(u8"渲染设置"))
    {
        static const std::vector<const char*> RenderMethodNames =
        {
            u8"金源渲染",
            u8"简易",
        };
        
        int RenderMethodIndex = static_cast<int>(m_RenderMethod);
        if (UI::combo(u8"渲染器", RenderMethodNames, RenderMethodIndex))
        {
            setRenderMethod(static_cast<ERenderMethod>(RenderMethodIndex));
        }

        bool SkyRendering = getSkyState();
        UI::toggle(u8"开启天空渲染", SkyRendering);
        setSkyState(SkyRendering);
    }

    UI::beginWindow(u8"纹理");

    if (!m_TextureImageSet.empty())
    {
        UI::combo(u8"选择纹理", m_TextureComboNameSet, m_CurTextureIndex);
        UI::slider(u8"缩放级别", m_TextureScale, 0.5f, 5.0f, "%.1f");
        const vk::CImage& Image = *m_TextureImageSet[m_CurTextureIndex];
        UI::image(Image, glm::vec2(Image.getWidth() * m_TextureScale, Image.getHeight() * m_TextureScale));
    }
    else
        UI::text(u8"暂无任何纹理");
    UI::endWindow();
    
}

void CRenderPassGoldSrc::_destroyV()
{
    destroyAndClear(m_pMainImage);
    destroyAndClear(m_pDepthImage);
    m_PipelineSet.destroy();
    destroyAndClear(m_pVertexBuffer);

    __destroySceneResources();
}

std::vector<VkCommandBuffer> CRenderPassGoldSrc::_requestCommandBuffersV()
{
    // sky
    CCommandBuffer::Ptr pSkyCmdBuffer = m_Command.getCommandBuffer("Sky");
    if (m_pRerecord->consume("Sky"))
    {
        _beginSecondaryCommand(pSkyCmdBuffer, m_RenderInfoDescriptor);
        if (m_EnableSky)
        {
            m_PipelineSet.Sky.recordCommand(pSkyCmdBuffer);
        }
        pSkyCmdBuffer->end();
    }

    // mesh
    CCommandBuffer::Ptr pMeshCmdBuffer = m_Command.getCommandBuffer("Mesh");
    if (m_pRerecord->consume("Mesh"))
    {
        _beginSecondaryCommand(pMeshCmdBuffer, m_RenderInfoDescriptor);
        if (isNonEmptyAndValid(m_pVertexBuffer))
        {
            pMeshCmdBuffer->bindVertexBuffer(*m_pVertexBuffer);
    
            if (m_RenderMethod == ERenderMethod::GOLDSRC)
            {
                m_PipelineSet.Normal.bind(pMeshCmdBuffer);
                m_PipelineSet.Normal.setOpacity(pMeshCmdBuffer, 1.0f);
            }
            else if (m_RenderMethod == ERenderMethod::SIMPLE)
            {
                m_PipelineSet.Simple.bind(pMeshCmdBuffer);
            }
            else
            {
                _SHOULD_NOT_GO_HERE;
            }

            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;
                if (m_ActorSegmentMap.find(pActor) == m_ActorSegmentMap.end()) continue;

                auto pTransform = pActor->getTransform();
                auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
                if (!pMeshRenderer) continue;

                auto pMesh = pMeshRenderer->getMesh();
                if (!pMesh) continue;

                if (m_RenderMethod == ERenderMethod::GOLDSRC)
                {
                    bool EnableLightmap = pMesh->getMeshDataV().hasLightmap();
                    m_PipelineSet.Normal.setLightmapState(pMeshCmdBuffer, EnableLightmap);
                }
                __drawMeshActor(pMeshCmdBuffer, pActor);
            }
        }
        pMeshCmdBuffer->end();
    }

    // sprite
    CCommandBuffer::Ptr pSpriteCmdBuffer = m_Command.getCommandBuffer("Sprite");
    if (m_pRerecord->consume("Sprite"))
    {
        _beginSecondaryCommand(pSpriteCmdBuffer, m_RenderInfoDescriptor);
        if (m_pSceneInfo && !m_pSceneInfo->SprSet.empty())
        {
            m_PipelineSet.Sprite.recordCommand(pSpriteCmdBuffer);
        }
        pSpriteCmdBuffer->end();
    }

    // icon
    CCommandBuffer::Ptr pIconCmdBuffer = m_Command.getCommandBuffer("Icon");
    if (m_pRerecord->consume("Icon"))
    {
        _beginSecondaryCommand(pIconCmdBuffer, m_RenderInfoDescriptor);
        if (m_pSceneInfo)
        {
            auto& PipelineIcon = m_PipelineSet.Icon;
            PipelineIcon.clear();
            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;

                auto pIconRenderer = pActor->getTransform()->findComponent<CComponentIconRenderer>();
                if (!pIconRenderer || !pIconRenderer->hasIcon()) continue;

                EIcon Icon = pIconRenderer->getIcon();
                glm::vec3 Position = pActor->getTransform()->getAbsoluteTranslate();
                glm::vec3 Scale = pActor->getTransform()->getAbsoluteScale();
                PipelineIcon.addIcon(Icon, Position, glm::max(Scale.x, glm::max(Scale.y, Scale.z)));
            }
            PipelineIcon.recordCommand(pIconCmdBuffer);
        }
        pIconCmdBuffer->end();
    }

    // text
    CCommandBuffer::Ptr pTextCmdBuffer = m_Command.getCommandBuffer("Text");
    auto& PipelineText = m_PipelineSet.Text;
    if (PipelineText.doesNeedRerecord())
    {
        _beginSecondaryCommand(pTextCmdBuffer, m_RenderInfoDescriptor);
        PipelineText.recordCommand(pTextCmdBuffer);
        pTextCmdBuffer->end();
    }

    // primary
    CCommandBuffer::Ptr pPrimaryCmdBuffer = _getCommandBuffer();
    _beginCommand(pPrimaryCmdBuffer);
    _beginRendering(pPrimaryCmdBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent, true));
    //pPrimaryCmdBuffer->execCommand(pInitCmdBuffer->get());
    pPrimaryCmdBuffer->execCommand(pSkyCmdBuffer->get());
    pPrimaryCmdBuffer->execCommand(pMeshCmdBuffer->get());
    pPrimaryCmdBuffer->execCommand(pSpriteCmdBuffer->get());
    pPrimaryCmdBuffer->execCommand(pIconCmdBuffer->get());
    pPrimaryCmdBuffer->execCommand(pTextCmdBuffer->get());
    _endRendering();
    _endCommand();
    
    return { pPrimaryCmdBuffer->get() };
}

void CRenderPassGoldSrc::__createSceneResources()
{
    __createTextureImages();
    __createLightmapImage();

    m_EnableSky = m_EnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;

    __updatePipelineResourceGoldSrc(m_PipelineSet.Normal);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAdditive);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAlphaTest);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendTextureAlpha);
    __updatePipelineResourceSimple(m_PipelineSet.Simple);
    __updatePipelineResourceSky(m_PipelineSet.Sky);
    __updatePipelineResourceSprite(m_PipelineSet.Sprite);

    // search all text renderer
    if (m_pSceneInfo)
    {
        auto& PipelineText = m_PipelineSet.Text;
        PipelineText.clearTextComponent();
        for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
        {
            auto pActor = m_pSceneInfo->pScene->getActor(i);
            if (!pActor->getVisible()) continue;

            auto pTextRenderer = pActor->getTransform()->findComponent<CComponentTextRenderer>();
            if (!pTextRenderer) continue;

            PipelineText.addTextComponent(pTextRenderer);
        }
    }

    m_pRerecord->requestRecordForAll();
}

void CRenderPassGoldSrc::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
    m_LightmapImage.destroy();
}

void CRenderPassGoldSrc::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.init(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            ImageUtils::createImageFromIOImage(*m_TextureImageSet[i], m_pDevice, m_pSceneInfo->TexImageSet[i]);
        }
    }
}

void CRenderPassGoldSrc::__createLightmapImage()
{
    if (m_pSceneInfo && m_pSceneInfo->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pSceneInfo->pLightmap->getCombinedLightmap();
        ImageUtils::createImageFromIOImage(m_LightmapImage, m_pDevice, pCombinedLightmapImage);
    }
}

void CRenderPassGoldSrc::__createVertexBuffer()
{
    destroyAndClear(m_pVertexBuffer);
    if (m_RenderMethod == ERenderMethod::GOLDSRC)
    {
        const auto& Pair = m_pSceneInfo->pScene->generateVertexBuffer<SGoldSrcPointData>(m_pDevice);

        m_pVertexBuffer = Pair.first;
        m_ActorSegmentMap = Pair.second;
    }
    else if (m_RenderMethod == ERenderMethod::SIMPLE)
    {
        const auto& Pair = m_pSceneInfo->pScene->generateVertexBuffer<SSimplePointData>(m_pDevice);

        m_pVertexBuffer = Pair.first;
        m_ActorSegmentMap = Pair.second;
    }
    else
    {
        _SHOULD_NOT_GO_HERE;
    }
}

void CRenderPassGoldSrc::__updateTextureView()
{
    size_t ImageNum = m_pSceneInfo->TexImageSet.size();
    _ASSERTE(ImageNum == m_TextureImageSet.size());
    m_CurTextureIndex = std::max<int>(0, std::min<int>(int(ImageNum), m_CurTextureIndex));
    m_TextureNameSet.resize(ImageNum);
    m_TextureComboNameSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_TextureNameSet[i] = std::to_string(i + 1) + ": " + m_pSceneInfo->TexImageSet[i]->getName();
        m_TextureComboNameSet[i] = m_TextureNameSet[i].c_str();
    }
}

size_t CRenderPassGoldSrc::__getActualTextureNum()
{
    size_t NumTexture = m_pSceneInfo ? m_pSceneInfo->TexImageSet.size() : 0;
    if (NumTexture > CPipelineNormal::MaxTextureNum)
    {
        Log::log("警告: 纹理数量 = (" + std::to_string(NumTexture) + ") 大于限制数量 (" + std::to_string(CPipelineNormal::MaxTextureNum) + "), 多出的纹理将被忽略");
        NumTexture = CPipelineNormal::MaxTextureNum;
    }
    return NumTexture;
}

void CRenderPassGoldSrc::__updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline)
{
    vPipeline.clearResources();
    vPipeline.setTextures(m_TextureImageSet);
    vPipeline.setLightmap(m_LightmapImage);
}

void CRenderPassGoldSrc::__updatePipelineResourceSimple(CPipelineSimple& vPipeline)
{
    vPipeline.setTextures(m_TextureImageSet);
}

void CRenderPassGoldSrc::__updatePipelineResourceSky(CPipelineSkybox& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (m_pSceneInfo->UseSkyBox && vPipeline.isValid())
        {
            vPipeline.setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
        }
    }
}

void CRenderPassGoldSrc::__updatePipelineResourceSprite(CPipelineSprite& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (!m_pSceneInfo->SprSet.empty() && vPipeline.isValid())
        {
            vPipeline.setSprites(m_pSceneInfo->SprSet);
        }
    }
}

void CRenderPassGoldSrc::__updateAllUniformBuffer()
{
    auto pCamera = m_pSceneInfo->pScene->getMainCamera();

    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Normal.updateUniformBuffer(Model, pCamera);
    m_PipelineSet.BlendTextureAlpha.updateUniformBuffer(Model, pCamera);
    m_PipelineSet.BlendAlphaTest.updateUniformBuffer(Model, pCamera);
    m_PipelineSet.BlendAdditive.updateUniformBuffer(Model, pCamera);
    m_PipelineSet.Simple.updateUniformBuffer(Model, pCamera);
    m_PipelineSet.Sprite.updateUniformBuffer(pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.updateUniformBuffer(pCamera);
    m_PipelineSet.Icon.updateUniformBuffer(pCamera);
    m_PipelineSet.Text.updateUniformBuffer(pCamera);
}
