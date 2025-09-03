#pragma once
#include "PointData.h"
#include "RenderPass.h"
#include "PipelineNormal.h"
#include "PipelineBlendAlpha.h"
#include "PipelineBlendAlphaTest.h"
#include "PipelineBlendAdditive.h"
#include "PipelineSimple.h"
#include "PipelineSkybox.h"
#include "PipelineSprite.h"
#include "PipelineIcon.h"
#include "PipelineText.h"
#include "Image.h"
#include "RerecordState.h"

class CRenderPassGoldSrc : public engine::IRenderPass
{
public:
    inline static const std::string Name = "GoldSrc";

    enum class ERenderMethod
    {
        GOLDSRC,
        SIMPLE
    };

    CRenderPassGoldSrc() = default;
    
    void rerecordAllCommand();

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vEnableSky)
    { 
        bool EnableSky = vEnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;
        if (m_EnableSky != EnableSky) m_pRerecord->requestRecord("Sky");
        m_EnableSky = EnableSky;
    }

    ERenderMethod getRenderMethod() const { return m_RenderMethod; }
    void setRenderMethod(ERenderMethod vMethod)
    {
        if (m_RenderMethod != vMethod)
        {
            m_RenderMethod = vMethod;
            m_pDevice->waitUntilIdle();
            __createVertexBuffer();
            m_pRerecord->requestRecord("Mesh");
        }
    }
    
protected:
    virtual void _initV() override;
    virtual sptr<CPortSet> _createPortSetV() override;
    virtual void _updateV() override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override;
    virtual void _destroyV() override;
    
    virtual std::vector<std::string> _getSecondaryCommandBufferNamesV() const override
    {
        return { "Init", "Mesh", "Sprite", "Icon", "Text", "Sky"};
    };

    virtual void _onSceneInfoSet(sptr<SSceneInfo> vScene) override;

private:
    void __createTextureImages();
    void __createLightmapImage();
    void __createVertexBuffer();

    void __createSceneResources();
    void __destroySceneResources();
    
    void __updateAllUniformBuffer();
    
    size_t __getActualTextureNum();
    void __updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline);
    void __updatePipelineResourceSimple(CPipelineSimple& vPipeline);
    void __updatePipelineResourceSky(CPipelineSkybox& vPipeline);
    void __updatePipelineResourceSprite(CPipelineSprite& vPipeline);
    void __updateTextureView();

    void __drawMeshActor(sptr<CCommandBuffer> vCommandBuffer, sptr<CActor> vActor)
    {
        _ASSERTE(m_ActorSegmentMap.find(vActor) != m_ActorSegmentMap.end());
        size_t SegIndex = m_ActorSegmentMap.at(vActor);
        const auto& Info = m_pVertexBuffer->getSegmentInfo(SegIndex);
        vCommandBuffer->draw(Info.First, Info.Num);
    }

    CRenderInfoDescriptor m_RenderInfoDescriptor;
    struct
    {
        CPipelineNormal Normal;
        CPipelineBlendAlpha BlendTextureAlpha;
        CPipelineBlendAlphaTest BlendAlphaTest;
        CPipelineBlendAdditive BlendAdditive;
        CPipelineSimple Simple;

        CPipelineSkybox Sky;
        CPipelineSprite Sprite;
        CPipelineIcon Icon;
        CPipelineText Text;
        
        void destroy()
        {
            Normal.destroy();
            BlendTextureAlpha.destroy();
            BlendAlphaTest.destroy();
            BlendAdditive.destroy();
            Simple.destroy();

            Sky.destroy();
            Sprite.destroy();
            Icon.destroy();
            Text.destroy();
        }
    } m_PipelineSet;
    
    vk::CPointerSet<vk::CImage> m_TextureImageSet;
    sptr<vk::CImage> m_pMainImage;
    sptr<vk::CImage> m_pDepthImage;
    vk::CImage m_LightmapImage;
    sptr<vk::CVertexBuffer> m_pVertexBuffer = nullptr;

    std::map<sptr<CActor>, size_t> m_ActorSegmentMap;
    
    bool m_EnableSky = true;
    ERenderMethod m_RenderMethod = ERenderMethod::GOLDSRC;

    // rerecord control
    sptr<CRerecordState> m_pRerecord = nullptr;

    // ÎÆÀí²é¿´
    int m_CurTextureIndex = 0;
    float m_TextureScale = 1.0f;
    std::vector<std::string> m_TextureNameSet;
    std::vector<const char*> m_TextureComboNameSet;
};
