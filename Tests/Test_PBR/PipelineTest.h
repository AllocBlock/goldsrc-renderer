#pragma once
#include "PipelineBase.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "MaterialPBR.h"

#include <glm/glm.hpp>
#include <array>

struct STestPointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec2 TexCoord;
    uint32_t MaterialIndex;

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
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(5);

        AttributeDescriptionSet[0].binding = 0;
        AttributeDescriptionSet[0].location = 0;
        AttributeDescriptionSet[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[0].offset = offsetof(STestPointData, Pos);

        AttributeDescriptionSet[1].binding = 0;
        AttributeDescriptionSet[1].location = 1;
        AttributeDescriptionSet[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[1].offset = offsetof(STestPointData, Normal);

        AttributeDescriptionSet[2].binding = 0;
        AttributeDescriptionSet[2].location = 2;
        AttributeDescriptionSet[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptionSet[2].offset = offsetof(STestPointData, Tangent);

        AttributeDescriptionSet[3].binding = 0;
        AttributeDescriptionSet[3].location = 3;
        AttributeDescriptionSet[3].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptionSet[3].offset = offsetof(STestPointData, TexCoord);

        AttributeDescriptionSet[4].binding = 0;
        AttributeDescriptionSet[4].location = 4;
        AttributeDescriptionSet[4].format = VK_FORMAT_R32_UINT;
        AttributeDescriptionSet[4].offset = offsetof(STestPointData, MaterialIndex);

        return AttributeDescriptionSet;
    }
};

class CPipelineTest : public CPipelineBase
{
public:
    void setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void setMaterialBuffer(std::shared_ptr<vk::CBuffer> vMaterialBuffer);
    void setTextures(const std::vector<vk::CImage::Ptr>& vColorSet, const std::vector<vk::CImage::Ptr>& vNormalSet, const std::vector<vk::CImage::Ptr>& vSpecularSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos,  bool vForceUseMat, bool vUseNormalMap, const SMaterialPBR& vMaterial);
    void destroy();

    bool isReady() {
        return m_pMaterialBuffer && m_pSkyBoxImage && m_TextureColorSet.size() > 0;
    }

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/shaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/shaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    std::vector<std::shared_ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<std::shared_ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
    std::shared_ptr<vk::CImage> m_pSkyBoxImage = nullptr;
    std::shared_ptr<vk::CImage> m_pPlaceholderImage = nullptr;
    std::shared_ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    std::vector<vk::CImage::Ptr> m_TextureColorSet;
    std::vector<vk::CImage::Ptr> m_TextureNormalSet;
    std::vector<vk::CImage::Ptr> m_TextureSpecularSet;
};

