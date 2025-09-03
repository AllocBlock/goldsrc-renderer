#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "MaterialPBR.h"
#include "VertexAttributeDescriptor.h"
#include "Sampler.h"

#include <glm/glm.hpp>

class CPipelinePBS : public IPipeline
{
public:

    struct SPointData
    {
        glm::vec3 Pos;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec2 TexCoord;
        uint32_t MaterialIndex;

        using PointData_t = SPointData;

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

    struct SControl
    {
        SMaterialPBR Material;
        bool ForceUseMat = false;
        bool UseColorTexture = true;
        bool UseNormalTexture = true;
        bool UseSpecularTexture = true;
    };

    void setMaterialBuffer(sptr<vk::CBuffer> vMaterialBuffer);
    void setTextures(const vk::CPointerSet<vk::CImage>& vColorSet, const vk::CPointerSet<vk::CImage>& vNormalSet, const vk::CPointerSet<vk::CImage>& vSpecularSet);
    void setSkyTexture(const sptr<CIOImage> vSkyImage, const sptr<CIOImage> vSkyIrrImage);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, const SControl& vControl);

    bool isReady() {
        return m_pMaterialBuffer && m_TextureColorSet.size() > 0 && m_SkyImage.isValid() && m_SkyIrrImage.isValid();
    }

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    vk::CSampler m_Sampler;
    vk::CSampler m_MipmapSampler;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage;
    sptr<vk::CBuffer> m_pMaterialBuffer = nullptr;
    std::vector<VkImageView> m_TextureColorSet;
    std::vector<VkImageView> m_TextureNormalSet;
    std::vector<VkImageView> m_TextureSpecularSet;
    vk::CImage m_SkyImage;
    vk::CImage m_SkyIrrImage;
    vk::CImage m_BRDFImage;

    const int m_MipmapLevelNum = 8;
};

