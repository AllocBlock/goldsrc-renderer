#pragma once
#include "Common.h"
#include "Scene.h"
#include "Camera.h"
#include "PipelineSkybox.h"
#include "PipelineDepthTest.h"
#include "PipelineBlend.h"
#include "PipelineLine.h"
#include "Descriptor.h"
#include "Command.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <optional>

enum class ERenderMethod
{
    DEFAULT,
    BSP
};

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

struct SPipelineSet
{
    CPipelineDepthTest TrianglesWithDepthTest;
    CPipelineBlend TrianglesWithBlend;
    CPipelineSkybox TrianglesSky;
    CPipelineLine GuiLines;

    void destroy()
    {
        TrianglesWithDepthTest.destroy();
        TrianglesWithBlend.destroy();
        TrianglesSky.destroy();
        GuiLines.destroy();
    }
};

class CVulkanRenderer
{
public:
    CVulkanRenderer();
    
    void init(const Common::SVulkanAppInfo& vAppInfo);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void update(uint32_t vImageIndex);
    void destroy();
    std::shared_ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(std::shared_ptr<SScene> vpScene);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void rerecordCommand();
    std::shared_ptr<CCamera> getCamera();
    size_t getRenderedObjectNum() const { return m_VisableObjectNum; }

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd); 

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState) { m_EnableSky = vSkyState && m_pScene && m_pScene->UseSkyBox; }
    bool getCullingState() const { return m_EnableCulling; }
    void setCullingState(bool vCullingState) { m_EnableCulling = vCullingState; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    void setFrustumCullingState(bool vFrustumCullingState) { m_EnableFrustumCulling = vFrustumCullingState; }
    bool getPVSState() const { return m_EnablePVS; }
    void setPVSState(bool vPVS) { m_EnablePVS = vPVS; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::vector<uint32_t> getRenderNodeList() const { return m_RenderNodeSet; }
    ERenderMethod getRenderMethod() const { return m_RenderMethod; }
    void setRenderMethod(ERenderMethod vRenderMethod) { m_RenderMethod = vRenderMethod; }

private:
    void __createRenderPass(bool vPresentLayout);
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
    void __renderModel(uint32_t vImageIndex, size_t vModelIndex, bool vBlend);
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __recordGuiCommandBuffers();
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const;
    std::pair<std::vector<size_t>, std::vector<size_t>> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount = 1);
    size_t __getActualTextureNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Common::SImagePack& voImagePack);
    void __updateDescriptorSets();

    std::vector<SGoldSrcPointData> __readPointData(std::shared_ptr<S3DObject> vpObject) const;

    Common::SVulkanAppInfo m_AppInfo;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    SPipelineSet m_PipelineSet = SPipelineSet();
    CCommand m_Command = CCommand();
    std::string m_SceneCommandName = "Scene";
    std::string m_GuiCommandName = "Gui";
    std::vector<VkFramebuffer> m_FramebufferSet;

    Common::SBufferPack m_VertexBufferPack;
    Common::SBufferPack m_IndexBufferPack;
    std::vector<Common::SImagePack> m_TextureImagePackSet;
    Common::SImagePack m_DepthImagePack;
    Common::SImagePack m_LightmapImagePack;

    std::shared_ptr<SScene> m_pScene;
    std::shared_ptr<CCamera> m_pCamera = nullptr;

    bool m_FramebufferResized = false;
    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    size_t m_VisableObjectNum = 0;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::vector<uint32_t> m_RenderNodeSet;
    ERenderMethod m_RenderMethod = ERenderMethod::BSP;

    size_t m_NumSwapchainImage = 0;
};