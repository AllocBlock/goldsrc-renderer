#pragma once
#include "Common.h"
#include "Device.h"
#include "UniformBuffer.h"
#include "Image.h"
#include "Sampler.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct SDescriptorInfo
{
    std::string Name;
    uint32_t Index;
    VkDescriptorType Type;
    uint32_t Size;
    VkShaderStageFlags Stage;
};

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

    void clear()
    {
        m_WriteInfoSet.clear();
    }

    void addWriteBuffer(size_t vTargetIndex, const vk::CBuffer& vBuffer)
    {
        _ASSERTE(vTargetIndex != std::numeric_limits<size_t>::max());
        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = vBuffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = vBuffer.getSize();
        m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {VertBufferInfo}, {} }));
    }

    void addWriteSampler(size_t vTargetIndex, VkSampler vSampler)
    {
        _ASSERTE(vSampler != VK_NULL_HANDLE);

        VkDescriptorImageInfo Info = {};
        Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Info.imageView =VK_NULL_HANDLE;
        Info.sampler = vSampler;
        m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {}, {Info} }));
    }

    void addWriteImagesAndSampler(size_t vTargetIndex, const vk::CHandleSet<vk::CImage>& vImageSet, VkSampler vSampler = VK_NULL_HANDLE)
    {
        _ASSERTE(vImageSet.isAllValid());
        addWriteImagesAndSampler(vTargetIndex, vImageSet, vSampler);
    }

    void addWriteImageAndSampler(size_t vTargetIndex, VkImageView vImageView = VK_NULL_HANDLE, VkSampler vSampler = VK_NULL_HANDLE)
    {
        _ASSERTE(!(vImageView == VK_NULL_HANDLE && vSampler == VK_NULL_HANDLE));

        VkDescriptorImageInfo Info = {};
        Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Info.imageView = vImageView;
        Info.sampler = vSampler;
        m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {}, {Info} }));
    }

    void addWriteImagesAndSampler(size_t vTargetIndex, const std::vector<VkImageView>& vImageViewSet, VkSampler vSampler = VK_NULL_HANDLE)
    {
        _ASSERTE(!vImageViewSet.empty());

        std::vector<VkDescriptorImageInfo> InfoSet(vImageViewSet.size());
        for (size_t i = 0; i < InfoSet.size(); ++i)
        {
            _ASSERTE(vImageViewSet[i] != VK_NULL_HANDLE);
            InfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InfoSet[i].imageView = vImageViewSet[i];
            InfoSet[i].sampler = vSampler;
        }
        m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {}, InfoSet }));
    }

private:
    std::vector<SDescriptorWriteInfoEntry> m_WriteInfoSet;
};

class CDescriptor
{
public:
    CDescriptor() = default;
    ~CDescriptor();

    void add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage);
    void createLayout(vk::CDevice::CPtr vDevice);
    
    const std::vector<VkDescriptorSet>& createDescriptorSetSet(size_t vImageNum);
    void update(size_t vSetIndex, const CDescriptorWriteInfo& vWriteInfo);
    void clear();
    VkDescriptorSetLayout getLayout() const;
    const std::vector<VkDescriptorPoolSize>& getPoolSizeSet() const;
    VkDescriptorSet getDescriptorSet(size_t vIndex) const;
    size_t getDescriptorSetNum() const;

private:
    void __createPool(size_t vImageNum);
    void __destroyPool();
    void __destroyLayout();
    void __destroySetSet();

    vk::CDevice::CPtr m_pDevice = nullptr;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<SDescriptorInfo> m_DescriptorInfoSet;
    std::vector<VkDescriptorPoolSize> m_PoolSizeSet;
    std::vector<VkDescriptorSet> m_DescriptorSetSet;
    VkDescriptorSetLayout m_DescriptorLayout = VK_NULL_HANDLE;
};

