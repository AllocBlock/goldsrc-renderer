#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"

class CPipelineEdge : public IPipeline
{
public:
    struct SPointData
    {
        glm::vec2 Pos;
        glm::vec2 TexCoord;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            Descriptor.add(_GET_ATTRIBUTE_INFO(TexCoord));
            return Descriptor.generate();
        }
    };

    void setInputImage(VkImageView vImageView, size_t vIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __initAllDescriptorSet();

    vk::CPointerSet<vk::CUniformBuffer> m_FragUbufferSet;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
    vk::CSampler m_Sampler;
};
