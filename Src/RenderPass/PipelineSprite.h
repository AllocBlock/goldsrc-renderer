#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "SceneInfo.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineSprite : public IPipeline
{
public:
    void setSprites(const std::vector<SGoldSrcSprite>& vSpriteSet);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    static const size_t MaxSpriteNum;

    struct SSpritePushConstant
    {
        uint32_t TexIndex = 0;
        uint32_t SpriteType = 0x00;
        float Scale = 1.0f;
        alignas(16) glm::vec3 Origin = glm::vec3(0.0f, 0.0f, 0.0f);
        alignas(16) glm::vec3 Angle = glm::vec3(0.0f, 0.0f, 0.0f);
    };

    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_PlaceholderImage;
    ptr<vk::CBuffer> m_pVertexDataBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;

    vk::CPointerSet<vk::CImage> m_SpriteImageSet;
    std::vector<SSpritePushConstant> m_SpriteSequence;
};

