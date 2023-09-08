#pragma once
#include "PointData.h"
#include "RenderPassSingleFrameBuffer.h"
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

class CRenderPassGoldSrc : public CRenderPassSingleFrameBuffer
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
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;
    
    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        voExtent = m_ScreenExtent;
        return voExtent != vk::ZeroExtent;
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
            m_pPortSet->getOutputPort("Depth")->getImageV(),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColorDepth;
    }

    virtual std::vector<std::string> _getExtraCommandBufferNamesV() const override
    {
        return { "Mesh", "Sprite", "Icon", "Text", "Sky"};
    };

    virtual void _onSceneInfoSet(ptr<SSceneInfo> vScene) override;

private:
    void __createTextureImages();
    void __createLightmapImage();
    void __createVertexBuffer();

    void __createSceneResources();
    void __destroySceneResources();
    
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    
    size_t __getActualTextureNum();
    void __updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline);
    void __updatePipelineResourceSimple(CPipelineSimple& vPipeline);
    void __updatePipelineResourceSky(CPipelineSkybox& vPipeline);
    void __updatePipelineResourceSprite(CPipelineSprite& vPipeline);
    void __updateTextureView();

    void __drawMeshActor(CCommandBuffer::Ptr vCommandBuffer, CActor::Ptr vActor)
    {
        _ASSERTE(m_ActorSegmentMap.find(vActor) != m_ActorSegmentMap.end());
        size_t SegIndex = m_ActorSegmentMap.at(vActor);
        const auto& Info = m_pVertexBuffer->getSegmentInfo(SegIndex);
        vCommandBuffer->draw(Info.First, Info.Num);
    }

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
    vk::CPointerSet<vk::CImage> m_MainImageSet;
    vk::CImage m_DepthImage;
    vk::CImage m_LightmapImage;
    vk::CVertexBuffer::Ptr m_pVertexBuffer = nullptr;

    std::map<CActor::Ptr, size_t> m_ActorSegmentMap;
    
    bool m_EnableSky = true;
    ERenderMethod m_RenderMethod = ERenderMethod::GOLDSRC;

    // rerecord control
    CRerecordState::Ptr m_pRerecord = nullptr;

    // ÎÆÀí²é¿´
    int m_CurTextureIndex = 0;
    float m_TextureScale = 1.0f;
    std::vector<std::string> m_TextureNameSet;
    std::vector<const char*> m_TextureComboNameSet;
};
