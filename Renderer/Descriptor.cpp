#include "Descriptor.h"

CDescriptor::~CDescriptor() { clear(); }

void CDescriptor::add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize,VkShaderStageFlags vStage)
{
    SDescriptorInfo Info = { vName, vIndex, vType, vSize, vStage };
    m_DescriptorInfoSet.emplace_back(std::move(Info));
    m_PoolSizeSet.emplace_back(VkDescriptorPoolSize({ vType, vSize }));
}

VkDescriptorSetLayout CDescriptor::createLayout(VkDevice vDevice)
{
    __destoryLayout();

    m_Device = vDevice;

    std::vector<VkDescriptorSetLayoutBinding> Bindings(m_DescriptorInfoSet.size());
    for (size_t i = 0; i < m_DescriptorInfoSet.size(); ++i)
    {
        Bindings[i].binding = m_DescriptorInfoSet[i].Index;
        Bindings[i].descriptorType = m_DescriptorInfoSet[i].Type;
        Bindings[i].descriptorCount = m_DescriptorInfoSet[i].Size;
        Bindings[i].stageFlags = m_DescriptorInfoSet[i].Stage;
    }

    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_PipelineLayout));

    return m_PipelineLayout;
}

const std::vector<VkDescriptorSet>& CDescriptor::createDescriptorSetSet(VkDescriptorPool vPool, size_t vNum)
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    _ASSERTE(m_PipelineLayout != VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayout> Layouts(vNum, m_PipelineLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = vPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(vNum);
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_DescriptorSetSet.resize(vNum);
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSetSet.data()));
    return m_DescriptorSetSet;
}

void CDescriptor::update(size_t vSetIndex, const std::vector<SDescriptorWriteInfo>& vWriteInfoSet)
{
    if (m_DescriptorInfoSet.size() != vWriteInfoSet.size())
        throw std::runtime_error(u8"����д������������������������һ��");

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

void CDescriptor::clear()
{
    m_DescriptorInfoSet.clear();
    m_PoolSizeSet.clear();
    m_DescriptorSetSet.clear();
    __destoryLayout();

    m_Device = VK_NULL_HANDLE;
}

VkDescriptorSetLayout CDescriptor::getLayout() const 
{
    return m_PipelineLayout; 
}

const std::vector<VkDescriptorPoolSize>& CDescriptor::getPoolSizeSet() const
{
    return m_PoolSizeSet;
}

VkDescriptorSet CDescriptor::getDescriptorSet(size_t vIndex) const
{
    _ASSERTE(vIndex < m_DescriptorSetSet.size());
    return m_DescriptorSetSet[vIndex];
}

size_t CDescriptor::getDescriptorSetNum() const
{
    return m_DescriptorSetSet.size();
}

void CDescriptor::__destoryLayout()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
}
