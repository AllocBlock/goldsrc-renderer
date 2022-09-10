#pragma once
#include "IPipeline.h"
#include "Scene.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

struct SSpritePushConstant
{
    uint32_t TexIndex = 0;
    uint32_t SpriteType = 0x00;
    float Scale = 1.0f;
    alignas(16) glm::vec3 Origin = glm::vec3(0.0f, 0.0f, 0.0f);
    alignas(16) glm::vec3 Angle = glm::vec3(0.0f, 0.0f, 0.0f);
};

class CPipelineSprite : public IPipeline
{
public:
    void setSprites(const std::vector<SGoldSrcSprite>& vSpriteSet);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/sprShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/sprShaderFrag.spv"; }

    void _initPushConstantV(VkCommandBuffer vCommandBuffer);
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _destroyV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override;

    static const size_t MaxSpriteNum;
private:
    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CImage> m_SpriteImageSet;
    vk::CImage m_PlaceholderImage;
    std::vector<SSpritePushConstant> m_SpriteSequence;
    vk::CBuffer m_VertexBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
};

