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
    ~CDescriptor() { clear(); }


    void add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage)
    {
        SDescriptorInfo Info = { vName, vIndex, vType, vSize, vStage };
        m_DescriptorInfoSet.emplace_back(std::move(Info));
        m_PoolSizeSet.emplace_back(VkDescriptorPoolSize({ vType, vSize }));
    }

    const std::vector<VkDescriptorPoolSize>& getPoolSizeSet() const
    {
        return m_PoolSizeSet;
    }

    VkDescriptorSetLayout createLayout(VkDevice vDevice)
    {
        __destoryLayout();

        m_Device = vDevice;

        std::vector<VkDescriptorSetLayoutBinding> Bindings(m_DescriptorInfoSet.size());
        for (size_t i = 0; i < m_DescriptorInfoSet.size(); ++i)
        {
            Bindings[i].binding         = m_DescriptorInfoSet[i].Index;
            Bindings[i].descriptorType  = m_DescriptorInfoSet[i].Type;
            Bindings[i].descriptorCount = m_DescriptorInfoSet[i].Size;
            Bindings[i].stageFlags      = m_DescriptorInfoSet[i].Stage;
        }

        VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
        LayoutInfo.pBindings = Bindings.data();

        ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_Layout));

        return m_Layout;
    }

    const std::vector<VkDescriptorSet>& createDescriptorSetSet(VkDescriptorPool vPool, size_t vNum)
    {
        _ASSERTE(m_Device != VK_NULL_HANDLE);
        _ASSERTE(m_Layout != VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayout> Layouts(vNum, m_Layout);

        VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
        DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescSetAllocInfo.descriptorPool = vPool;
        DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(vNum);
        DescSetAllocInfo.pSetLayouts = Layouts.data();

        m_DescriptorSetSet.resize(vNum);
        ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSetSet.data()));
        return m_DescriptorSetSet;
    }

    void update(size_t vSetIndex, const std::vector<SDescriptorWriteInfo>& vWriteInfoSet)
    {
        if (m_DescriptorInfoSet.size() != vWriteInfoSet.size())
            throw std::runtime_error(u8"错误，写入描述符的数量和描述符不一致");

        std::vector<VkWriteDescriptorSet> DescriptorWrites(m_DescriptorInfoSet.size());
        for (size_t i = 0; i < m_DescriptorInfoSet.size(); ++i)
        {
            DescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[i].dstSet = m_DescriptorSetSet[vSetIndex];
            DescriptorWrites[i].dstBinding = m_DescriptorInfoSet[i].Index;
            DescriptorWrites[i].dstArrayElement = 0;
            DescriptorWrites[i].descriptorType = m_DescriptorInfoSet[i].Type;
            DescriptorWrites[i].descriptorCount = m_DescriptorInfoSet[i].Size;
            DescriptorWrites[i].pBufferInfo = vWriteInfoSet[i].BufferInfoSet.empty() ? nullptr : vWriteInfoSet[i].BufferInfoSet.data();
            DescriptorWrites[i].pImageInfo = vWriteInfoSet[i].ImageInfoSet.empty() ? nullptr : vWriteInfoSet[i].ImageInfoSet.data();
        }

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }

    void clear()
    {
        m_DescriptorInfoSet.clear();
        m_PoolSizeSet.clear();
        m_DescriptorSetSet.clear();
        __destoryLayout();

        m_Device = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayout getLayout() const { return m_Layout; }
    VkDescriptorSet getDescriptorSet(size_t vIndex) const
    { 
        _ASSERTE(vIndex < m_DescriptorSetSet.size());
        return m_DescriptorSetSet[vIndex];
    }

private:
    void __destoryLayout()
    {
        if (m_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;
        }
    }

    VkDevice m_Device = VK_NULL_HANDLE;
    std::vector<SDescriptorInfo> m_DescriptorInfoSet;
    std::vector<VkDescriptorPoolSize> m_PoolSizeSet;
    std::vector<VkDescriptorSet> m_DescriptorSetSet;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
};

