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
    bool getCullingState() const { return m_EnableCulling; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    bool getPVSState() const { return m_EnablePVS; }
    bool getBSPState() const { return m_EnableBSP; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::set<uint32_t> getRenderedNodeList() const { return m_RenderNodeSet; }
    std::set<size_t> getRenderedObjectSet() const { return m_RenderedObjectSet; }

    void setSkyState(bool vEnableSky)
    { 
        bool EnableSky = vEnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;
        if (m_EnableSky != EnableSky) rerecordCommand();
        m_EnableSky = EnableSky;
    }

    void setCullingState(bool vEnableCulling) 
    { 
        if (m_EnableCulling != vEnableCulling) rerecordCommand();
        m_EnableCulling = vEnableCulling;
    }

    void setFrustumCullingState(bool vEnableFrustumCulling) 
    { 
        if (m_EnableFrustumCulling != vEnableFrustumCulling) rerecordCommand();
        m_EnableFrustumCulling = vEnableFrustumCulling;
    }

    void setPVSState(bool vPVS) 
    { 
        bool EnablePVS = vPVS && m_pSceneInfo && m_pSceneInfo->BspPvs.LeafNum > 0;
        if (m_EnablePVS != EnablePVS) rerecordCommand();
        m_EnablePVS = EnablePVS;
    }

    void setBSPState(bool vEnableBSP) 
    { 
        bool EnableBSP = vEnableBSP && m_pSceneInfo && !m_pSceneInfo->BspTree.Nodes.empty();
        if (!EnableBSP)
            m_RenderNodeSet.clear();
        if (m_EnableBSP != EnableBSP) rerecordCommand();
        m_EnableBSP = EnableBSP;
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

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __renderModels(uint32_t vImageIndex);
    void __renderModel(uint32_t vImageIndex, size_t vModelIndex);
    void __renderPointEntities(uint32_t vImageIndex);
    void __renderSprites(uint32_t vImageIndex);
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    std::vector<size_t> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    void __recordSpriteRenderCommand(uint32_t vImageIndex);
    
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

        void destroy()
        {
            Normal.destroy();
            BlendTextureAlpha.destroy();
            BlendAlphaTest.destroy();
            BlendAdditive.destroy();
            Sprite.destroy();
            Sky.destroy();
        }
    } m_PipelineSet;
    
    vk::CPointerSet<vk::CImage> m_TextureImageSet;
    CDynamicTextureCreator m_DepthImageManager;
    vk::CImage m_LightmapImage;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    std::set<size_t> m_RenderedObjectSet;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    bool m_EnableBSP = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::set<uint32_t> m_RenderNodeSet;

    // ÎÆÀí²é¿´
    int m_CurTextureIndex = 0;
    float m_TextureScale = 1.0f;
    std::vector<std::string> m_TextureNameSet;
    std::vector<const char*> m_TextureComboNameSet;
};
