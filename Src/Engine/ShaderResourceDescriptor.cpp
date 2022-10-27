#include "ShaderResourceDescriptor.h"
#include "Vulkan.h"

using namespace vk;

void CDescriptorWriteInfo::clear()
{
    m_WriteInfoSet.clear();
}

void CDescriptorWriteInfo::addWriteBuffer(size_t vTargetIndex, const vk::CBuffer& vBuffer)
{
    _ASSERTE(vTargetIndex != std::numeric_limits<size_t>::max());
    VkDescriptorBufferInfo VertBufferInfo = {};
    VertBufferInfo.buffer = vBuffer;
    VertBufferInfo.offset = 0;
    VertBufferInfo.range = vBuffer.getSize();
    m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {VertBufferInfo}, {} }));
}

void CDescriptorWriteInfo::addWriteSampler(size_t vTargetIndex, VkSampler vSampler)
{
    _ASSERTE(vSampler != VK_NULL_HANDLE);

    VkDescriptorImageInfo Info = {};
    Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Info.imageView =VK_NULL_HANDLE;
    Info.sampler = vSampler;
    m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {}, {Info} }));
}

void CDescriptorWriteInfo::addWriteImageAndSampler(size_t vTargetIndex, VkImageView vImageView, VkSampler vSampler)
{
    _ASSERTE(!(vImageView == VK_NULL_HANDLE && vSampler == VK_NULL_HANDLE));

    VkDescriptorImageInfo Info = {};
    Info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Info.imageView = vImageView;
    Info.sampler = vSampler;
    m_WriteInfoSet.emplace_back(SDescriptorWriteInfoEntry({ vTargetIndex, {}, {Info} }));
}

void CDescriptorWriteInfo::addWriteImagesAndSampler(size_t vTargetIndex, const std::vector<VkImageView>& vImageViewSet,
    VkSampler vSampler)
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

void CDescriptorWriteInfo::addWriteImagesAndSampler(size_t vTargetIndex, const std::vector<vk::CImage>& vImageSet,
    VkSampler vSampler)
{
    std::vector<VkImageView> ImageViewSet(vImageSet.size());

    for (size_t i = 0; i < ImageViewSet.size(); ++i)
    {
        ImageViewSet[i] = vImageSet[i];
    }

    addWriteImagesAndSampler(vTargetIndex, ImageViewSet, vSampler);
}

void CDescriptorWriteInfo::addWriteImagesAndSampler(size_t vTargetIndex, const vk::CPointerSet<vk::CImage>& vImageSet,
    VkSampler vSampler)
{
    _ASSERTE(!vImageSet.empty() && vImageSet.isAllValid());

    std::vector<VkImageView> ImageViewSet(vImageSet.size());

    for (size_t i = 0; i < ImageViewSet.size(); ++i)
    {
        ImageViewSet[i] = *vImageSet[i];
    }

    addWriteImagesAndSampler(vTargetIndex, ImageViewSet, vSampler);
}

CShaderResourceDescriptor::~CShaderResourceDescriptor() { clear(); }

void CShaderResourceDescriptor::add(std::string vName, uint32_t vIndex, VkDescriptorType vType, uint32_t vSize, VkShaderStageFlags vStage)
{
    SDescriptorInfo Info = { vName, vIndex, vType, vSize, vStage };
    m_DescriptorInfoSet.emplace_back(std::move(Info));
    m_PoolSizeSet.emplace_back(VkDescriptorPoolSize({ vType, vSize }));
}

void CShaderResourceDescriptor::createLayout(CDevice::CPtr vDevice)
{
    __destroyLayout();

    m_pDevice = vDevice;

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

    vk::checkError(vkCreateDescriptorSetLayout(*m_pDevice, &LayoutInfo, nullptr, &m_DescriptorLayout));
}

const std::vector<VkDescriptorSet>& CShaderResourceDescriptor::createDescriptorSetSet(size_t vImageNum)
{
    _ASSERTE(vImageNum > 0);
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
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
    vk::checkError(vkAllocateDescriptorSets(*m_pDevice, &DescSetAllocInfo, m_DescriptorSetSet.data()));
    return m_DescriptorSetSet;
}

void CShaderResourceDescriptor::update(size_t vSetIndex, const CDescriptorWriteInfo& vWriteInfo)
{
    const std::vector<SDescriptorWriteInfoEntry>& vWriteInfoSet = vWriteInfo.get();
    if (m_DescriptorInfoSet.size() != vWriteInfoSet.size())
        throw std::runtime_error(u8"错误，写入描述符的数量和描述符不一致");

    std::vector<VkWriteDescriptorSet> DescriptorWrites(vWriteInfoSet.size());
    for (size_t i = 0; i < vWriteInfoSet.size(); ++i)
    {
        size_t TargetIndex = vWriteInfoSet[i].TargetIndex;
        const SDescriptorInfo& TargetInfo = m_DescriptorInfoSet[TargetIndex];

        DescriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescriptorWrites[i].dstSet = m_DescriptorSetSet[vSetIndex];
        DescriptorWrites[i].dstBinding = TargetInfo.Index;
        DescriptorWrites[i].dstArrayElement = 0;
        DescriptorWrites[i].descriptorType = TargetInfo.Type;
        DescriptorWrites[i].descriptorCount = TargetInfo.Size;
        DescriptorWrites[i].pBufferInfo = vWriteInfoSet[i].BufferInfoSet.empty() ? nullptr : vWriteInfoSet[i].BufferInfoSet.data();
        DescriptorWrites[i].pImageInfo = vWriteInfoSet[i].ImageInfoSet.empty() ? nullptr : vWriteInfoSet[i].ImageInfoSet.data();
        _ASSERTE(!vWriteInfoSet[i].BufferInfoSet.empty() || !vWriteInfoSet[i].ImageInfoSet.empty());
    }

    vkUpdateDescriptorSets(*m_pDevice, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
}

void CShaderResourceDescriptor::clear()
{
    m_DescriptorInfoSet.clear();
    m_PoolSizeSet.clear();

    __destroySetSet();
    __destroyLayout();
    __destroyPool();
    m_pDevice = nullptr;
}

VkDescriptorSetLayout CShaderResourceDescriptor::getLayout() const 
{
    return m_DescriptorLayout; 
}

const std::vector<VkDescriptorPoolSize>& CShaderResourceDescriptor::getPoolSizeSet() const
{
    return m_PoolSizeSet;
}

VkDescriptorSet CShaderResourceDescriptor::getDescriptorSet(size_t vIndex) const
{
    _ASSERTE(vIndex < m_DescriptorSetSet.size());
    return m_DescriptorSetSet[vIndex];
}

size_t CShaderResourceDescriptor::getDescriptorSetNum() const
{
    return m_DescriptorSetSet.size();
}

void CShaderResourceDescriptor::__createPool(size_t vImageNum)
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    __destroyPool();

    auto PoolSize = m_PoolSizeSet;
    for (auto& Size : PoolSize)
        Size.descriptorCount *= static_cast<uint32_t>(vImageNum);

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSize.size());
    PoolInfo.pPoolSizes = PoolSize.data();
    PoolInfo.maxSets = static_cast<uint32_t>(vImageNum);

    vk::checkError(vkCreateDescriptorPool(*m_pDevice, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CShaderResourceDescriptor::__destroyPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(*m_pDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void CShaderResourceDescriptor::__destroyLayout()
{
    if (m_DescriptorLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(*m_pDevice, m_DescriptorLayout, nullptr);
        m_DescriptorLayout = VK_NULL_HANDLE;
    }
}

void CShaderResourceDescriptor::__destroySetSet()
{
    if (!m_DescriptorSetSet.empty())
    {
        m_DescriptorSetSet.clear();
    }
}
