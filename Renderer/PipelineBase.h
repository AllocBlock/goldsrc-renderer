#pragma once
#include "Common.h"

#include <filesystem>
#include <optional>
#include <vulkan/vulkan.h> 

struct CPipelineBase
{
public:
    CPipelineBase() = delete;
    CPipelineBase(std::filesystem::path vVertShaderPath, std::filesystem::path vFragShaderPath) : m_VertShaderPath(vVertShaderPath), m_FragShaderPath(vFragShaderPath) {}

    void create(VkDevice vDevice, VkRenderPass vRenderPass, uint32_t vSubpass, VkDescriptorSetLayout vDescriptorSetLayout, VkExtent2D vExtent);
    void destory();
    void bind(VkCommandBuffer vCommandBuffer, VkDescriptorSet vDescSet);

    template <typename T>
    void pushConstant(VkCommandBuffer vCommandBuffer, VkShaderStageFlags vState, T vPushConstant)
    {
        vkCmdPushConstants(vCommandBuffer, m_Layout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

protected:
    virtual VkPipelineShaderStageCreateInfo _getShadeStageInfoV(VkShaderModule vVertModule, VkShaderModule vFragModule);
    virtual VkPipelineVertexInputStateCreateInfo _getVertexInputStageInfoV();
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV();
    virtual VkPipelineViewportStateCreateInfo _getViewportStageInfoV(VkExtent2D vExtent);
    virtual VkPipelineRasterizationStateCreateInfo _getRasterizationStageInfoV();
    virtual VkPipelineMultisampleStateCreateInfo _getMultisampleStageInfoV();
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV();
    virtual VkPipelineColorBlendStateCreateInfo _getColorBlendInfoV();
    virtual VkPipelineDynamicStateCreateInfo _getDynamicStateInfoV();
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV();

    static VkPipelineShaderStageCreateInfo          getDefaultShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule);
    static VkPipelineVertexInputStateCreateInfo     getDefaultVertexInputStageInfo();
    static VkPipelineInputAssemblyStateCreateInfo   getDefaultInputAssemblyStageInfo();
    static VkPipelineViewportStateCreateInfo        getDefaultViewportStageInfo(VkExtent2D vExtent);
    static VkPipelineRasterizationStateCreateInfo   getDefaultRasterizationStageInfo();
    static VkPipelineMultisampleStateCreateInfo     getDefaultMultisampleStageInfo();
    static VkPipelineDepthStencilStateCreateInfo    getDefaultDepthStencilInfo();
    static VkPipelineColorBlendStateCreateInfo      getDefaultColorBlendInfo();
    static VkPipelineDynamicStateCreateInfo         getDefaultDynamicStateInfo();
    static std::vector<VkPushConstantRange>         getDefaultPushConstantRangeSet();

    std::filesystem::path m_VertShaderPath;
    std::filesystem::path m_FragShaderPath;

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};
