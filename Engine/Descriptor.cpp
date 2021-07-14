#include "Descriptor.h"
#include "Vulkan.h"

CDescriptor::~CDescriptor() { clear(); }

void CDescriptor::add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage)
{
    SDescriptorInfo Info = { vName, vIndex, vType, vSize, vStage };
    m_DescriptorInfoSet.emplace_back(std::move(Info));
    m_PoolSizeSet.emplace_back(VkDescriptorPoolSize({ vType, vSize }));
}

void CDescriptor::createLayout(VkDevice vDevice)
{
    __destroyLayout();

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

    Vulkan::checkError(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_DescriptorLayout));
}

const std::vector<VkDescriptorSet>& CDescriptor::createDescriptorSetSet(size_t vImageNum)
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    _ASSERTE(m_DescriptorLayout != VK_NULL_HANDLE);

    if (m_DescriptorPool == VK_NULL_HANDLE || m_DescriptorSetSet.size() > vImageNum)
        __createPool(vImageNum);

    std::vector<VkDescriptorSetLayout> Layouts(vImageNum, m_DescriptorLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(vImageNum);
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    __destroySetSet();
    m_DescriptorSetSet.resize(vImageNum);
    Vulkan::checkError(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSetSet.data()));
    return m_DescriptorSetSet;
}

void CDescriptor::update(size_t vSetIndex, const std::vector<SDescriptorWriteInfo>& vWriteInfoSet)
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

void CDescriptor::clear()
{
    m_DescriptorInfoSet.clear();
    m_PoolSizeSet.clear();

    __destroySetSet();
    __destroyLayout();
    __destroyPool();
    m_Device = VK_NULL_HANDLE;
}

VkDescriptorSetLayout CDescriptor::getLayout() const 
{
    return m_DescriptorLayout; 
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

void CDescriptor::__createPool(size_t vImageNum)
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    __destroyPool();

    auto PoolSize = m_PoolSizeSet;
    for (auto& Size : PoolSize)
        Size.descriptorCount *= static_cast<uint32_t>(vImageNum);

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSize.size());
    PoolInfo.pPoolSizes = PoolSize.data();
    PoolInfo.maxSets = static_cast<uint32_t>(vImageNum);

    Vulkan::checkError(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CDescriptor::__destroyPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void CDescriptor::__destroyLayout()
{
    if (m_DescriptorLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorLayout, nullptr);
        m_DescriptorLayout = VK_NULL_HANDLE;
    }
}

void CDescriptor::__destroySetSet()
{
    if (!m_DescriptorSetSet.empty())
    {
        m_DescriptorSetSet.clear();
    }
}
