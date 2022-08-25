#include "VertexAttributeDescriptor.h"

std::vector<VkVertexInputAttributeDescription> CVertexAttributeDescriptor::generate()
{
    _ASSERTE(m_FormatSet.size() == m_OffsetSet.size());
    size_t Size = m_FormatSet.size();
    std::vector<VkVertexInputAttributeDescription> AttributeDescriptionSet(Size);

    const uint32_t Binding = 0; // TODO: no need for other binding for now, so add it when needed

    for (uint32_t i = 0; i < Size; ++i)
    {
        AttributeDescriptionSet[i].binding = Binding;
        AttributeDescriptionSet[i].location = i;
        AttributeDescriptionSet[i].format = m_FormatSet[i];
        AttributeDescriptionSet[i].offset = m_OffsetSet[i];
    }

    return AttributeDescriptionSet;
}

void CVertexAttributeDescriptor::add(VkFormat vFormat, uint32_t vOffset)
{
    m_FormatSet.emplace_back(vFormat);
    m_OffsetSet.emplace_back(vOffset);
}

void CVertexAttributeDescriptor::clear()
{
    m_FormatSet.clear();
    m_OffsetSet.clear();
}

VkVertexInputBindingDescription createBindingDescription(size_t vSize)
{
    VkVertexInputBindingDescription BindingDescription = {};
    BindingDescription.binding = 0;
    BindingDescription.stride = vSize;
    BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return BindingDescription;
}