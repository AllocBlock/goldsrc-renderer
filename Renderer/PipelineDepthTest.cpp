#include "PipelineDepthTest.h"

size_t CPipelineDepthTest::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

struct SPushConstant
{
    VkBool32 UseLightmap = VK_FALSE;
    float Opacity = 1.0;
};

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

struct SUniformBufferObjectFrag
{
    alignas(16) glm::vec3 Eye;
};

void CPipelineDepthTest::createResources(size_t vImageNum)
{

}

void CPipelineDepthTest::updateTexture(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap)
{
    __updateDescriptorSet(vTextureSet, vLightmap);
}

void CPipelineDepthTest::setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable)
{
    if (m_EnableLightmap == vEnable) return;
    else
    {
        m_EnableLightmap = vEnable;
        __updatePushConstant(vCommandBuffer, vEnable, m_Opacity);
    }
}

void CPipelineDepthTest::setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity)
{
    if (m_Opacity == vOpacity) return;
    else
    {
        m_Opacity = vOpacity;
        __updatePushConstant(vCommandBuffer, m_EnableLightmap, vOpacity);
    }
}

void CPipelineDepthTest::destroy()
{
    for (size_t i = 0; i < m_VertUniformBufferPackSet.size(); ++i)
    {
        m_VertUniformBufferPackSet[i].destroy(m_Device);
        m_FragUniformBufferPackSet[i].destroy(m_Device);
    }
    m_VertUniformBufferPackSet.clear();
    m_FragUniformBufferPackSet.clear();

    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    m_TextureSampler = VK_NULL_HANDLE;
}


VkPipelineVertexInputStateCreateInfo CPipelineDepthTest::_getVertexInputStageInfoV()
{
    const auto& Binding = SPointData::getBindingDescription();
    const auto& AttributeSet = SPointData::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo Info = CPipelineBase::getDefaultVertexInputStageInfo();
    Info.vertexBindingDescriptionCount = 1;
    Info.pVertexBindingDescriptions = &Binding;
    Info.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeSet.size());
    Info.pVertexAttributeDescriptions = AttributeSet.data();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineDepthTest::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

VkPipelineDepthStencilStateCreateInfo CPipelineDepthTest::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

VkPipelineDynamicStateCreateInfo CPipelineDepthTest::_getDynamicStateInfoV()
{
    std::vector<VkDynamicState> EnabledDynamicStates =
    {
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfo = {};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(EnabledDynamicStates.size());
    DynamicStateInfo.pDynamicStates = EnabledDynamicStates.data();

    return DynamicStateInfo;
}

std::vector<VkPushConstantRange> CPipelineDepthTest::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    return { PushConstantInfo };
}

void CPipelineDepthTest::_createDescriptor(VkDescriptorPool vPool, uint32_t vImageNum)
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, CPipelineDepthTest::MaxTextureNum, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Lightmap", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
    m_Descriptor.createDescriptorSetSet(vPool, vImageNum);
}

void CPipelineDepthTest::__updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity)
{
    SPushConstant PushConstant;
    PushConstant.UseLightmap = vEnableLightmap;
    PushConstant.Opacity = vOpacity;
    pushConstant(vCommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
}

void CPipelineDepthTest::__updateDescriptorSet(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap)
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPackSet[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo}, {} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPackSet[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo}, {} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        //const size_t NumTexture = __getActualTextureNum();
        const size_t NumTexture = vTextureSet.size();

        std::vector<VkDescriptorImageInfo> TexImageInfoSet(CPipelineDepthTest::MaxTextureNum);
        for (size_t i = 0; i < m_MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                {
                    TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfoSet[i].imageView = m_PlaceholderImagePack.ImageView;
                    TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
                }
                else
                {
                    TexImageInfoSet[i] = TexImageInfoSet[0];
                }
            }
            else
            {
                TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                TexImageInfoSet[i].imageView = vTextureSet[i];
                TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
            }
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageInfoSet }));

        VkDescriptorImageInfo LightmapImageInfo = {};
        LightmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        LightmapImageInfo.imageView = vLightmap == VK_NULL_HANDLE ? m_PlaceholderImagePack.ImageView : vLightmap;
        LightmapImageInfo.sampler = VK_NULL_HANDLE;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {LightmapImageInfo} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CPipelineDepthTest::__createUniformBuffers(size_t vImageNum)
{
    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferPackSet.resize(vImageNum);
    m_FragUniformBufferPackSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Common::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPackSet[i].Buffer, m_VertUniformBufferPackSet[i].Memory);
        Common::createBuffer(m_PhysicalDevice, m_Device, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPackSet[i].Buffer, m_FragUniformBufferPackSet[i].Memory);
    }
}

void CPipelineDepthTest::__updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)
{
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPackSet[vImageIndex].Memory);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = vEyePos;

    ck(vkMapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferPackSet[vImageIndex].Memory);
}