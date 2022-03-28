#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"

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

class CPipelineTest : public IPipeline
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

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<std::shared_ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<std::shared_ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
    std::shared_ptr<vk::CImage> m_pSkyBoxImage = nullptr;
    std::shared_ptr<vk::CImage> m_pPlaceholderImage = nullptr;
};

