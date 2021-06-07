#pragma once
#include "Common.h"

#include <filesystem>
#include <optional>
#include <vulkan/vulkan.h> 

struct CPipeline
{
public:
    CPipeline() = delete;
    CPipeline(VkPrimitiveTopology vPrimitiveToplogy, std::filesystem::path vVertShaderPath, std::filesystem::path vFragShaderPath) : m_PrimitiveToplogy(vPrimitiveToplogy), m_VertShaderPath(vVertShaderPath), m_FragShaderPath(vFragShaderPath) {}

    void create(VkDevice vDevice, VkRenderPass vRenderPass, VkVertexInputBindingDescription vInputBindingDescription, std::vector<VkVertexInputAttributeDescription> vInputAttributeDescriptions, VkExtent2D vExtent, VkDescriptorSetLayout vDescriptorSetLayout, VkPipelineDepthStencilStateCreateInfo vDepthStencilInfo, VkPipelineColorBlendStateCreateInfo vBlendInfo, uint32_t vSubpass = 0, std::optional<VkPipelineDynamicStateCreateInfo> vDynamicStateInfo = std::nullopt, std::optional<VkPushConstantRange> vPushConstantState = std::nullopt);
    void destory();
    void bind(VkCommandBuffer vCommandBuffer, VkDescriptorSet vDescSet);

    template <typename T>
    void pushConstant(VkCommandBuffer vCommandBuffer, VkShaderStageFlags vState, T vPushConstant)
    {
        vkCmdPushConstants(vCommandBuffer, m_Layout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

private:
    VkPrimitiveTopology m_PrimitiveToplogy = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    std::filesystem::path m_VertShaderPath;
    std::filesystem::path m_FragShaderPath;

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
};
