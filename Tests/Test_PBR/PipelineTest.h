#pragma once
#include "PipelineBase.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "MaterialPBR.h"
#include "VertexAttributeDescriptor.h"

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
        VertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(STestPointData, Pos));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(STestPointData, Normal));
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(STestPointData, Tangent));
        Descriptor.add(VK_FORMAT_R32G32_SFLOAT,    offsetof(STestPointData, TexCoord));
        Descriptor.add(VK_FORMAT_R32_UINT,         offsetof(STestPointData, MaterialIndex));
        return Descriptor.generate();
    }
};

class CPipelineTest : public CPipelineBase
{
public:
    struct SControl
    {
        SMaterialPBR Material;
        bool ForceUseMat = false;
        bool UseColorTexture = true;
        bool UseNormalTexture = true;
        bool UseSpecularTexture = true;
    };

    void setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void setMaterialBuffer(std::shared_ptr<vk::CBuffer> vMaterialBuffer);
    void setTextures(const std::vector<vk::CImage::Ptr>& vColorSet, const std::vector<vk::CImage::Ptr>& vNormalSet, const std::vector<vk::CImage::Ptr>& vSpecularSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, const SControl& vControl);
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

