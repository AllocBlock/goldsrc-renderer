#include "Pipeline.h"
#include "Common.h"

void IPipeline::create(vk::CDevice::CPtr vDevice, VkRenderPass vRenderPass, VkExtent2D vExtent, uint32_t vSubpass)
{
    destroy();

    m_pDevice = vDevice;
    m_Extent = vExtent;

    _initShaderResourceDescriptorV();

    CPipelineDescriptor PipelineDescriptor = _getPipelineDescriptionV();

    auto VertShaderCode = Common::readFileAsChar(PipelineDescriptor.getVertShaderPath());
    auto FragShaderCode = Common::readFileAsChar(PipelineDescriptor.getFragShaderPath());

    VkShaderModule VertShaderModule = m_pDevice->createShaderModule(VertShaderCode);
    VkShaderModule FragShaderModule = m_pDevice->createShaderModule(FragShaderCode);

    const auto& ShadeInfoSet = PipelineDescriptor.getShadeStageInfo(VertShaderModule, FragShaderModule);

    const auto& VertexInputInfo = PipelineDescriptor.getVertexInputStageInfo();
    const auto& InputAssemblyInfo = PipelineDescriptor.getInputAssemblyStageInfo();
    const auto& ViewportInfo = PipelineDescriptor.getViewportStageInfo(m_Extent);
    const auto& RasterizationInfo = PipelineDescriptor.getRasterizationStageInfo();
    const auto& MultisampleInfo = PipelineDescriptor.getMultisampleStageInfo();
    const auto& DepthStencilInfo = PipelineDescriptor.getDepthStencilInfo();
    const auto& ColorBlendInfo = PipelineDescriptor.getColorBlendInfo();
    const auto& DynamicInfo = PipelineDescriptor.getEnabledDynamicStateInfo();

    const auto& PushConstantRangeSet = PipelineDescriptor.getPushConstantRangeSet();

    const auto& Layout = m_ShaderResourceDescriptor.getLayout();
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &Layout;
    PipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(PushConstantRangeSet.size());
    PipelineLayoutInfo.pPushConstantRanges = PushConstantRangeSet.data();

    vk::checkError(vkCreatePipelineLayout(*m_pDevice, &PipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = static_cast<uint32_t>(ShadeInfoSet.size());
    PipelineInfo.pStages = ShadeInfoSet.data();
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportInfo;
    PipelineInfo.pRasterizationState = &RasterizationInfo;
    PipelineInfo.pMultisampleState = &MultisampleInfo;
    PipelineInfo.pDepthStencilState = &DepthStencilInfo;
    PipelineInfo.pColorBlendState = &ColorBlendInfo;
    PipelineInfo.pDynamicState = &DynamicInfo;
    PipelineInfo.layout = m_PipelineLayout;
    PipelineInfo.renderPass = vRenderPass;
    PipelineInfo.subpass = vSubpass;

    vk::checkError(vkCreateGraphicsPipelines(*m_pDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_Pipeline));
    m_pDevice->destroyShaderModule(VertShaderModule);
    m_pDevice->destroyShaderModule(FragShaderModule);

    _createV();
}

void IPipeline::setImageNum(uint32_t vImageNum)
{
    if (vImageNum == m_ImageNum) return;
    m_ImageNum = vImageNum;
    m_ShaderResourceDescriptor.createDescriptorSetSet(vImageNum);
    _createResourceV(vImageNum);
}

void IPipeline::destroy()
{
    _destroyV();

    m_ImageNum = 0;
    
    if (m_pDevice == VK_NULL_HANDLE) return;
    m_ShaderResourceDescriptor.clear();

    vkDestroyPipeline(*m_pDevice, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(*m_pDevice, m_PipelineLayout, nullptr);
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Pipeline = VK_NULL_HANDLE;
    m_pDevice = VK_NULL_HANDLE;
}

void IPipeline::bind(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    const auto& DescriptorSet = m_ShaderResourceDescriptor.getDescriptorSet(vImageIndex);
    vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
    _initPushConstantV(vCommandBuffer);
}