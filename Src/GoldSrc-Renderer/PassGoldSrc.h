#pragma once
#include "PointData.h"
#include "PassScene.h"
#include "FrameBuffer.h"
#include "PipelineSkybox.h"
#include "PipelineNormal.h"
#include "PipelineBlendAlpha.h"
#include "PipelineBlendAlphaTest.h"
#include "PipelineBlendAdditive.h"
#include "PipelineSprite.h"
#include "PipelineIcon.h"
#include "Image.h"
#include "DynamicResourceManager.h"

#include <vulkan/vulkan.h>
#include <optional>
#include <set>

class CSceneGoldSrcRenderPass : public CRenderPassSceneTyped<SGoldSrcPointData>
{
public:
    CSceneGoldSrcRenderPass() = default;
    
    void rerecordCommand();

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vEnableSky)
    { 
        bool EnableSky = vEnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;
        if (m_EnableSky != EnableSky) rerecordCommand();
        m_EnableSky = EnableSky;
    }
    
protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;
    
    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Main", voExtent);
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

    virtual void _loadSceneV(ptr<SSceneInfoGoldSrc> vScene) override;

private:
    void __createTextureImages();
    void __createLightmapImage();

    void __createSceneResources();
    void __destroySceneResources();
    
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __drawActor(uint32_t vImageIndex, CActor::Ptr vActor);
    
    size_t __getActualTextureNum();
    void __updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline);
    void __updatePipelineResourceSky(CPipelineSkybox& vPipeline);
    void __updatePipelineResourceSprite(CPipelineSprite& vPipeline);
    void __updateTextureView();

    struct
    {
        CDynamicPipeline<CPipelineNormal> Normal;
        CDynamicPipeline<CPipelineBlendAlpha> BlendTextureAlpha;
        CDynamicPipeline<CPipelineBlendAlphaTest> BlendAlphaTest;
        CDynamicPipeline<CPipelineBlendAdditive> BlendAdditive;
        CDynamicPipeline<CPipelineSprite> Sprite;
        CDynamicPipeline<CPipelineSkybox> Sky;
        CDynamicPipeline<CPipelineIcon> Icon;

        void update(const vk::SPassUpdateState& vUpdateState)
        {
            Sky.updateV(vUpdateState);
            Normal.updateV(vUpdateState);
            BlendTextureAlpha.updateV(vUpdateState);
            BlendAlphaTest.updateV(vUpdateState);
            BlendAdditive.updateV(vUpdateState);
            Sprite.updateV(vUpdateState);
            Icon.updateV(vUpdateState);
        }

        void destroy()
        {
            Normal.destroy();
            BlendTextureAlpha.destroy();
            BlendAlphaTest.destroy();
            BlendAdditive.destroy();
            Sprite.destroy();
            Sky.destroy();
            Icon.destroy();
        }
    } m_PipelineSet;
    
    vk::CPointerSet<vk::CImage> m_TextureImageSet;
    CDynamicTextureCreator m_DepthImageManager;
    vk::CImage m_LightmapImage;

    size_t m_RerecordCommandTimes = 0;
    bool m_EnableSky = true;

    // �����鿴
    int m_CurTextureIndex = 0;
    float m_TextureScale = 1.0f;
    std::vector<std::string> m_TextureNameSet;
    std::vector<const char*> m_TextureComboNameSet;
};
