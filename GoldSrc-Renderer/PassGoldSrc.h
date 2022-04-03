#pragma once
#include "ScenePass.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "PipelineSkybox.h"
#include "PipelineDepthTest.h"
#include "PipelineBlendAlpha.h"
#include "PipelineBlendAlphaTest.h"
#include "PipelineBlendAdditive.h"
#include "PipelineLine.h"
#include "PipelineSprite.h"
#include "Descriptor.h"
#include "Command.h"
#include "Image.h"
#include "Buffer.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <optional>
#include <set>

struct SPipelineSet
{
    CPipelineDepthTest DepthTest;
    CPipelineBlendAlpha BlendTextureAlpha;
    CPipelineBlendAlphaTest BlendAlphaTest;
    CPipelineBlendAdditive BlendAdditive;
    CPipelineSprite Sprite;
    CPipelineSkybox Sky;
    CPipelineLine GuiLines;

    void destroy()
    {
        DepthTest.destroy();
        BlendTextureAlpha.destroy();
        BlendAlphaTest.destroy();
        BlendAdditive.destroy();
        Sprite.destroy();
        Sky.destroy();
        GuiLines.destroy();
    }
};

class CSceneGoldSrcRenderPass : public CSceneRenderPass
{
public:
    CSceneGoldSrcRenderPass() = default;
    
    void rerecordCommand();

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd); 

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
        bool EnableSky = vEnableSky && m_pScene && m_pScene->UseSkyBox;
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
        bool EnablePVS = vPVS && m_pScene && m_pScene->BspPvs.LeafNum > 0;
        if (m_EnablePVS != EnablePVS) rerecordCommand();
        m_EnablePVS = EnablePVS;
    }
    void setBSPState(bool vEnableBSP) 
    { 
        bool EnableBSP = vEnableBSP && m_pScene && !m_pScene->BspTree.Nodes.empty();
        if (!EnableBSP)
            m_RenderNodeSet.clear();
        if (m_EnableBSP != EnableBSP) rerecordCommand();
        m_EnableBSP = EnableBSP;
    }

protected:
    virtual void _initV() override;
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _loadSceneV(ptr<SScene> vScene);

private:
    void __createRenderPass();
    void __createGraphicsPipelines();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createLightmapImage();
    void __createVertexBuffer();
    void __createIndexBuffer();
    
    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __renderModels(uint32_t vImageIndex);
    void __renderModel(uint32_t vImageIndex, size_t vModelIndex);
    void __renderPointEntities(uint32_t vImageIndex);
    void __renderSprites(uint32_t vImageIndex);
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __recordGuiCommandBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(ptr<C3DObject> vpObject, const SFrustum& vFrustum) const;
    std::vector<size_t> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    void __recordSpriteRenderCommand(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    size_t __getActualTextureNum();
    void __updateDescriptorSets();

    std::vector<SGoldSrcPointData> __readPointData(ptr<C3DObjectGoldSrc> vpObject) const;

    SPipelineSet m_PipelineSet = SPipelineSet();
    CCommand m_Command = CCommand();
    std::string m_SceneCommandName = "Scene";
    std::string m_GuiCommandName = "Gui";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;

    ptr<vk::CBuffer> m_pVertexBuffer;
    ptr<vk::CBuffer> m_pIndexBuffer;
    std::vector<vk::CImage::Ptr> m_TextureImageSet;
    vk::CImage::Ptr m_pDepthImage;
    vk::CImage::Ptr m_pLightmapImage;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    std::set<size_t> m_RenderedObjectSet;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    bool m_EnableBSP = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::set<uint32_t> m_RenderNodeSet;

    size_t m_NumSwapchainImage = 0;
};