#pragma once
#include "PipelineBase.h"
#include "Common.h"
#include "Descriptor.h"

#include <glm/glm.hpp>

struct SSimplePointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SSimplePointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(1);

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SSimplePointData, Pos);

        return AttributeDescriptions;
    }
};

class CPipelineSkybox : public CPipelineBase
{
public:
    void destroy();
    void createResources(size_t vImageNum);

protected:
    virtual void _createDescriptor(VkDescriptorPool vPool, uint32_t vImageNum) override;
    virtual VkPipelineVertexInputStateCreateInfo _getVertexInputStageInfoV() override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;

private:
    void __createTextureSampler();
    void __updateDescriptorSet();

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    Common::SImagePack m_SkyBoxImagePack; // cubemap
    Common::SBufferPack m_VertexDataPack;
    size_t m_VertexNum = 0;
    std::vector<Common::SBufferPack> m_VertUniformBufferPacks;
    std::vector<Common::SBufferPack> m_FragUniformBufferPacks;
};

