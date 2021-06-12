#pragma once
#include "PipelineBase.h"
#include "IOImage.h"

#include <glm/glm.hpp>
#include <array>

struct STestPointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(STestPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(2);

        AttributeDescriptionSet[0].binding = 0;
        AttributeDescriptionSet[0].location = 0;
        AttributeDescriptionSet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[0].offset = offsetof(STestPointData, Pos);

        AttributeDescriptionSet[1].binding = 0;
        AttributeDescriptionSet[1].location = 1;
        AttributeDescriptionSet[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[1].offset = offsetof(STestPointData, Normal);

        return AttributeDescriptionSet;
    }
};

class CPipelineTest : public CPipelineBase
{
public:
    void setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos);
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/vert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/frag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();
    void __updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity);

    bool m_EnableLightmap = false;
    float m_Opacity = 1.0f;

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<Common::SBufferPack> m_VertUniformBufferPackSet;
    std::vector<Common::SBufferPack> m_FragUniformBufferPackSet;
    Common::SImagePack m_SkyBoxImagePack;
    Common::SImagePack m_PlaceholderImagePack;
};

