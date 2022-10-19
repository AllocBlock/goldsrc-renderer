#pragma once
#include "IGUI.h"
#include "Vulkan.h"
#include "Descriptor.h"
#include "Device.h"

#include <filesystem>
#include <vulkan/vulkan.h> 

struct IPipeline : public IGUI
{
public:
    IPipeline() = default;
    virtual ~IPipeline() = default;

    void create(vk::CDevice::CPtr vDevice, VkRenderPass vRenderPass, VkExtent2D vExtent, uint32_t vSubpass = 0);
    void setImageNum(size_t vImageNum);
    void destroy();
    void bind(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    bool isValid() const { return m_Pipeline != VK_NULL_HANDLE; }

    template <typename T>
    void pushConstant(VkCommandBuffer vCommandBuffer, VkShaderStageFlags vState, T vPushConstant)
    {
        vkCmdPushConstants(vCommandBuffer, m_PipelineLayout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

    const CDescriptor& getDescriptor() const { return m_Descriptor; }

protected:
    virtual void _createV() {};
    virtual void _renderUIV() override {};
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
    virtual void _destroyV() {};

    static std::vector<VkPipelineShaderStageCreateInfo>          getDefaultShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule);
    static VkPipelineInputAssemblyStateCreateInfo   getDefaultInputAssemblyStageInfo();
    static VkPipelineRasterizationStateCreateInfo   getDefaultRasterizationStageInfo();
    static VkPipelineMultisampleStateCreateInfo     getDefaultMultisampleStageInfo();
    static VkPipelineDepthStencilStateCreateInfo    getDefaultDepthStencilInfo();
    static std::vector<VkPushConstantRange>         getDefaultPushConstantRangeSet();

    size_t m_ImageNum = 0;
    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };

    CDescriptor m_Descriptor;

    vk::CDevice::CPtr m_pDevice = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
};
