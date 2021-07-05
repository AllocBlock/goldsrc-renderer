#pragma once
#include "Common.h"

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

struct SDescriptorWriteInfo
{
    std::vector<VkDescriptorBufferInfo> BufferInfoSet;
    std::vector<VkDescriptorImageInfo> ImageInfoSet;
};

class CDescriptor
{
public:
    CDescriptor() = default;
    ~CDescriptor();

    void add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage);
    void createLayout(VkDevice vDevice);
    
    const std::vector<VkDescriptorSet>& createDescriptorSetSet(size_t vImageNum);
    void update(size_t vSetIndex, const std::vector<SDescriptorWriteInfo>& vWriteInfoSet);
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

    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<SDescriptorInfo> m_DescriptorInfoSet;
    std::vector<VkDescriptorPoolSize> m_PoolSizeSet;
    std::vector<VkDescriptorSet> m_DescriptorSetSet;
    VkDescriptorSetLayout m_DescriptorLayout = VK_NULL_HANDLE;
};

