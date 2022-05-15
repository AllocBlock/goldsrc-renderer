#pragma once
#include "Pipeline.h"
#include "PointData.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineDepthTest : public IPipeline
{
public:
    void setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable);
    void setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity);
    void updateDescriptorSet(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera);
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    void _initPushConstantV(VkCommandBuffer vCommandBuffer);
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/shaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/shaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual std::vector<VkDynamicState> _getEnabledDynamicSetV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;
private:
    void __destroyResources();
    void __updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity);
    
    bool m_EnableLightmap = false;
    float m_Opacity = 1.0f;

    vk::CSampler m_Sampler;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
    vk::CImage::Ptr m_pPlaceholderImage;
};