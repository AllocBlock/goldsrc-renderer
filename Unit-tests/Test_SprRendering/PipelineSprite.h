#pragma once
#include "PipelineBase.h"
#include "Common.h"
#include "Descriptor.h"
#include "IOImage.h"
#include "Scene.h"

#include <glm/glm.hpp>
#include <array>

struct SSpritePushConstant
{
    uint32_t TexIndex = 0;
    uint32_t SpriteType = 0x00;
    float Scale = 1.0f;
    alignas(16) glm::vec3 Origin = glm::vec3(0.0f, 0.0f, 0.0f);
    alignas(16) glm::vec3 Angle = glm::vec3(0.0f, 0.0f, 0.0f);
};

class CPipelineSprite : public CPipelineBase
{
public:
    void destroy();
    void setSprites(const std::vector<SGoldSrcSprite>& vSpriteSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, glm::vec3 vEyeDirection);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/sprShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/sprShaderFrag.spv"; }

    void _initPushConstantV(VkCommandBuffer vCommandBuffer);
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment) override;

    static const size_t MaxSpriteNum;
private:
    void __updateDescriptorSet();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vImage, Vulkan::SImagePack& voImagePack);

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<Vulkan::SImagePack> m_SpriteImagePackSet;
    Vulkan::SImagePack m_PlaceholderImagePack;
    std::vector<SSpritePushConstant> m_SpriteSequence;
    Vulkan::SBufferPack m_VertexDataPack;
    size_t m_VertexNum = 0;
    std::vector<Vulkan::SBufferPack> m_VertUniformBufferPacks;
};
