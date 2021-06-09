#pragma once
#include "PipelineBase.h"

#include <glm/glm.hpp>

struct SPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec2 LightmapCoord;
    uint32_t TexIndex;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(6);

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SPointData, Pos);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(SPointData, Color);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(SPointData, Normal);

        AttributeDescriptions[3].binding = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[3].offset = offsetof(SPointData, TexCoord);

        AttributeDescriptions[4].binding = 0;
        AttributeDescriptions[4].location = 4;
        AttributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[4].offset = offsetof(SPointData, LightmapCoord);

        AttributeDescriptions[5].binding = 0;
        AttributeDescriptions[5].location = 5;
        AttributeDescriptions[5].format = VK_FORMAT_R32_UINT;
        AttributeDescriptions[5].offset = offsetof(SPointData, TexIndex);

        return AttributeDescriptions;
    }
};

class CPipelineDepthTest : public CPipelineBase
{
public:
    void createResources(size_t vImageNum);
    void setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable);
    void setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity);
    void updateTexture(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap);
    void update();
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _createDescriptor(VkDescriptorPool vPool, uint32_t vImageNum) override;
    virtual VkPipelineVertexInputStateCreateInfo _getVertexInputStageInfoV() override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual VkPipelineDynamicStateCreateInfo _getDynamicStateInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;
private:
    void __updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity);
    void __updateDescriptorSet(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap);

    void __createUniformBuffers(size_t vImageNum);
    void __updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)

    bool m_EnableLightmap = false;
    bool m_Opacity = 1.0f;

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<Common::SBufferPack> m_VertUniformBufferPackSet;
    std::vector<Common::SBufferPack> m_FragUniformBufferPackSet;
    Common::SImagePack m_PlaceholderImagePack;
};