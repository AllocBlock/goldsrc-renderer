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
    void setTextures(const vk::CHandleSet<vk::CImage>& vColorSet, const vk::CHandleSet<vk::CImage>& vNormalSet, const vk::CHandleSet<vk::CImage>& vSpecularSet);
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
    vk::CHandleSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CHandleSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage = nullptr;
    ptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    vk::CHandleSet<vk::CImage> m_TextureColorSet;
    vk::CHandleSet<vk::CImage> m_TextureNormalSet;
    vk::CHandleSet<vk::CImage> m_TextureSpecularSet;
    vk::CImage m_SkyImage = nullptr;
    vk::CImage m_SkyIrrImage = nullptr;
    vk::CImage m_BRDFImage = nullptr;

    const int m_MipmapLevelNum = 8;
};

