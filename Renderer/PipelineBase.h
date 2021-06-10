#pragma once
#include "Common.h"
#include "Descriptor.h"

#include <filesystem>
#include <optional>
#include <vulkan/vulkan.h> 

struct CPipelineBase
{
public:
    CPipelineBase() = default;

    void create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkRenderPass vRenderPass, VkExtent2D vExtent, uint32_t vSubpass = 0);
    void setImageNum(size_t vImageNum);
    void destroy();
    void bind(VkCommandBuffer vCommandBuffer, size_t vImageIndex);

    template <typename T>
    void pushConstant(VkCommandBuffer vCommandBuffer, VkShaderStageFlags vState, T vPushConstant)
    {
        vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

    const CDescriptor& getDescriptor() const { return m_Descriptor; }

protected:
    virtual std::filesystem::path _getVertShaderPathV() = 0;
    virtual std::filesystem::path _getFragShaderPathV() = 0;
    virtual void _createResourceV(size_t vImageNum) = 0;
    virtual void _initDescriptorV() = 0;
    virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer);
    virtual std::vector<VkPipelineShaderStageCreateInfo> _getShadeStageInfoV(VkShaderModule vVertModule, VkShaderModule vFragModule);
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) = 0;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV();
    virtual void _getViewportStageInfoV(VkExtent2D vExtent, VkViewport& voViewport, VkRect2D& voScissor);
    virtual VkPipelineRasterizationStateCreateInfo _getRasterizationStageInfoV();
    virtual VkPipelineMultisampleStateCreateInfo _getMultisampleStageInfoV();
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV();
    virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment);
    virtual std::vector<VkDynamicState> _getEnabledDynamicSetV();
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV();

    static std::vector<VkPipelineShaderStageCreateInfo>          getDefaultShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule);
    static VkPipelineInputAssemblyStateCreateInfo   getDefaultInputAssemblyStageInfo();
    static VkPipelineRasterizationStateCreateInfo   getDefaultRasterizationStageInfo();
    static VkPipelineMultisampleStateCreateInfo     getDefaultMultisampleStageInfo();
    static VkPipelineDepthStencilStateCreateInfo    getDefaultDepthStencilInfo();
    static std::vector<VkPushConstantRange>         getDefaultPushConstantRangeSet();

    size_t m_ImageNum = 0;

    CDescriptor m_Descriptor;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};
