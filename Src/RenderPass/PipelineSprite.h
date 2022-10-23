#pragma once
#include "Pipeline.h"
#include "SceneInfoGoldSrc.h"
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
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer) override;
    virtual void _destroyV() override;

    static const size_t MaxSpriteNum;
private:
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
    vk::CPointerSet<vk::CImage> m_SpriteImageSet;
    vk::CImage m_PlaceholderImage;
    std::vector<SSpritePushConstant> m_SpriteSequence;
    vk::CBuffer m_VertexBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
};

