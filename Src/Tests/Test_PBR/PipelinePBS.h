#pragma once
#include "IPipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "MaterialPBR.h"
#include "VertexAttributeDescriptor.h"
#include "Sampler.h"

#include <glm/glm.hpp>
#include <array>

struct SPBSPointData
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec2 TexCoord;
    uint32_t MaterialIndex;

    using PointData_t = SPBSPointData;

    _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Normal));
        Descriptor.add(_GET_ATTRIBUTE_INFO(Tangent));
        Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
        Descriptor.add(_GET_ATTRIBUTE_INFO(MaterialIndex));
        return Descriptor.generate();
    }
};

class CPipelinePBS : public IPipeline
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

    void setMaterialBuffer(ptr<vk::CBuffer> vMaterialBuffer);
    void setTextures(const std::vector<vk::CImage::Ptr>& vColorSet, const std::vector<vk::CImage::Ptr>& vNormalSet, const std::vector<vk::CImage::Ptr>& vSpecularSet);
    void setSkyTexture(const CIOImage::Ptr vSkyImage, const CIOImage::Ptr vSkyIrrImage);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, const SControl& vControl);
    void destroy();

    bool isReady() {
        return m_pMaterialBuffer && m_TextureColorSet.size() > 0 && m_pSkyImage && m_pSkyIrrImage;
    }

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/shaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/shaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    vk::CSampler m_Sampler;
    vk::CSampler m_MipmapSampler;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
    ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    std::vector<vk::CImage::Ptr> m_TextureColorSet;
    std::vector<vk::CImage::Ptr> m_TextureNormalSet;
    std::vector<vk::CImage::Ptr> m_TextureSpecularSet;
    vk::CImage::Ptr m_pSkyImage = nullptr;
    vk::CImage::Ptr m_pSkyIrrImage = nullptr;
    vk::CImage::Ptr m_pBRDFImage = nullptr;

    const int m_MipmapLevelNum = 8;
};

