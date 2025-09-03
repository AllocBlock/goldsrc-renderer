#pragma once
#include "Device.h"
#include "UniformBuffer.h"
#include "Image.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct SDescriptorWriteInfoEntry
{
    size_t TargetIndex = std::numeric_limits<size_t>::max();
    std::vector<VkDescriptorBufferInfo> BufferInfoSet;
    std::vector<VkDescriptorImageInfo> ImageInfoSet;
};

class CDescriptorWriteInfo
{
public:
    const std::vector<SDescriptorWriteInfoEntry>& get() const { return m_WriteInfoSet; }

    void clear();
    void addWriteBuffer(size_t vTargetIndex, const vk::CBuffer& vBuffer);
    void addWriteSampler(size_t vTargetIndex, VkSampler vSampler);
    void addWriteImageAndSampler(size_t vTargetIndex, VkImageView vImageView = VK_NULL_HANDLE, VkSampler vSampler = VK_NULL_HANDLE);
    void addWriteImagesAndSampler(size_t vTargetIndex, const std::vector<VkImageView>& vImageViewSet, VkSampler vSampler = VK_NULL_HANDLE);
    void addWriteImagesAndSampler(size_t vTargetIndex, const std::vector<vk::CImage>& vImageSet, VkSampler vSampler = VK_NULL_HANDLE);
    void addWriteImagesAndSampler(size_t vTargetIndex, const vk::CPointerSet<vk::CImage>& vImageSet, VkSampler vSampler = VK_NULL_HANDLE);
    void addWriteInputAttachment(size_t vTargetIndex, VkImageView vImageView);

private:
    std::vector<SDescriptorWriteInfoEntry> m_WriteInfoSet;
};

class CShaderResourceDescriptor
{
public:
    CShaderResourceDescriptor() = default;
    ~CShaderResourceDescriptor();

    void add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage);
    void createLayout(cptr<vk::CDevice> vDevice);
    
    const VkDescriptorSet createDescriptorSet();
    void update(const CDescriptorWriteInfo& vWriteInfo);
    void clear();
    VkDescriptorSetLayout getLayout() const;
    const std::vector<VkDescriptorPoolSize>& getPoolSizeSet() const;
    VkDescriptorSet getDescriptorSet() const;

    bool isReady(); // is shader resource ready (num > 0, and all are updated)

private:
    struct SDescriptorInfo
    {
        std::string Name;
        uint32_t Index;
        VkDescriptorType Type;
        uint32_t Size;
        VkShaderStageFlags Stage;
    };

    void __createPool();
    void __destroyPool();
    void __destroyLayout();
    void __destroyDescriptorSet();

    cptr<vk::CDevice> m_pDevice = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<SDescriptorInfo> m_DescriptorInfoSet;
    std::vector<VkDescriptorPoolSize> m_PoolSizeSet;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorLayout = VK_NULL_HANDLE;
};
