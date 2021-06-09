#include "PipelineBase.h"

VkPipelineShaderStageCreateInfo CPipelineBase::getDefaultShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule)
{
    VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
    VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertShaderStageInfo.module = vVertModule;
    VertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
    FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragShaderStageInfo.module = vFragModule;
    FragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };
}

VkPipelineVertexInputStateCreateInfo CPipelineBase::getDefaultVertexInputStageInfo()
{
    // default no vertex input
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 0;
    VertexInputInfo.vertexAttributeDescriptionCount = 0;

    return VertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo CPipelineBase::getDefaultInputAssemblyStageInfo()
{
    // default triangle list
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    return InputAssemblyInfo;
}

VkPipelineViewportStateCreateInfo CPipelineBase::getDefaultViewportStageInfo(VkExtent2D vExtent)
{
    // default full screen
    VkViewport Viewport = {};
    Viewport.width = static_cast<float>(vExtent.width);
    Viewport.height = static_cast<float>(vExtent.height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;

    VkRect2D Scissor = {};
    Scissor.offset = { 0, 0 };
    Scissor.extent = vExtent;

    VkPipelineViewportStateCreateInfo ViewportStateInfo = {};
    ViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateInfo.viewportCount = 1;
    ViewportStateInfo.pViewports = &Viewport;
    ViewportStateInfo.scissorCount = 1;
    ViewportStateInfo.pScissors = &Scissor;

    return ViewportStateInfo;
}

VkPipelineRasterizationStateCreateInfo CPipelineBase::getDefaultRasterizationStageInfo()
{
    // default counter clockwise, back culling, no depth bias
    VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
    RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.depthBiasEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    return RasterizerInfo;
}

VkPipelineMultisampleStateCreateInfo CPipelineBase::getDefaultMultisampleStageInfo()
{
    // default no msaa
    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return Multisampling;
}

VkPipelineDepthStencilStateCreateInfo CPipelineBase::getDefaultDepthStencilInfo()
{
    // default enable depth test
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

VkPipelineColorBlendStateCreateInfo CPipelineBase::getDefaultColorBlendInfo()
{
    // default disable blending
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;

    return ColorBlendInfo;
}

VkPipelineDynamicStateCreateInfo CPipelineBase::getDefaultDynamicStateInfo()
{
    // default no dynamic state
    VkPipelineDynamicStateCreateInfo DynamicStateInfo = {};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = 0;

    return DynamicStateInfo;
}

std::vector<VkPushConstantRange> CPipelineBase::getDefaultPushConstantRangeSet()
{
    // default no push constant
    return {};
}

void CPipelineBase::create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkRenderPass vRenderPass, VkExtent2D vExtent, uint32_t vSubpass)
{
    destroy();

    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;

    auto VertShaderCode = Common::readFileAsChar(m_VertShaderPath);
    auto FragShaderCode = Common::readFileAsChar(m_FragShaderPath);

    VkShaderModule VertShaderModule = Common::createShaderModule(m_Device, VertShaderCode);
    VkShaderModule FragShaderModule = Common::createShaderModule(m_Device, FragShaderCode);

    const auto& PushConstantRangeSet = _getPushConstantRangeSetV();

    const auto& Layout = m_Descriptor.getLayout();
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &Layout;
    PipelineLayoutInfo.pushConstantRangeCount = PushConstantRangeSet.size();
    PipelineLayoutInfo.pPushConstantRanges = PushConstantRangeSet.data();

    ck(vkCreatePipelineLayout(m_Device, &PipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = &_getShadeStageInfoV(VertShaderModule, FragShaderModule);
    PipelineInfo.pVertexInputState = &_getVertexInputStageInfoV();
    PipelineInfo.pInputAssemblyState = &_getInputAssemblyStageInfoV();
    PipelineInfo.pViewportState = &_getViewportStageInfoV(vExtent);
    PipelineInfo.pRasterizationState = &_getRasterizationStageInfoV();
    PipelineInfo.pMultisampleState = &_getMultisampleStageInfoV();
    PipelineInfo.pDepthStencilState = &_getDepthStencilInfoV();
    PipelineInfo.pColorBlendState = &_getColorBlendInfoV();
    PipelineInfo.pDynamicState = &_getDynamicStateInfoV();
    PipelineInfo.layout = m_PipelineLayout;
    PipelineInfo.renderPass = vRenderPass;
    PipelineInfo.subpass = vSubpass;

    ck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_Pipeline));

    vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
}

void CPipelineBase::destroy()
{
    if (m_Device == VK_NULL_HANDLE) return;
    m_Descriptor.clear();

    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    m_PipelineLayout = VK_NULL_HANDLE;
    m_Pipeline = VK_NULL_HANDLE;
    m_Device = VK_NULL_HANDLE;
    m_PhysicalDevice = VK_NULL_HANDLE;
}

void CPipelineBase::bind(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
{
    const auto& DescriptorSet = m_Descriptor.getDescriptorSet(vImageIndex);
    vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
}

VkPipelineShaderStageCreateInfo CPipelineBase::_getShadeStageInfoV(VkShaderModule vVertModule, VkShaderModule vFragModule)
{
    return CPipelineBase::getDefaultShadeStageInfo(vVertModule, vFragModule);
}

VkPipelineVertexInputStateCreateInfo CPipelineBase::_getVertexInputStageInfoV()
{
    return CPipelineBase::getDefaultVertexInputStageInfo();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineBase::_getInputAssemblyStageInfoV()
{
    return CPipelineBase::getDefaultInputAssemblyStageInfo();
}

VkPipelineViewportStateCreateInfo CPipelineBase::_getViewportStageInfoV(VkExtent2D vExtent)
{
    return CPipelineBase::getDefaultViewportStageInfo(vExtent);
}

VkPipelineRasterizationStateCreateInfo CPipelineBase::_getRasterizationStageInfoV()
{
    return CPipelineBase::getDefaultRasterizationStageInfo();
}

VkPipelineMultisampleStateCreateInfo CPipelineBase::_getMultisampleStageInfoV()
{
    return CPipelineBase::getDefaultMultisampleStageInfo();
}

VkPipelineDepthStencilStateCreateInfo CPipelineBase::_getDepthStencilInfoV()
{
    return CPipelineBase::getDefaultDepthStencilInfo();
}

VkPipelineColorBlendStateCreateInfo CPipelineBase::_getColorBlendInfoV()
{
    return CPipelineBase::getDefaultColorBlendInfo();
}

VkPipelineDynamicStateCreateInfo CPipelineBase::_getDynamicStateInfoV()
{
    return CPipelineBase::getDefaultDynamicStateInfo();
}

std::vector<VkPushConstantRange> CPipelineBase::_getPushConstantRangeSetV()
{
    return CPipelineBase::getDefaultPushConstantRangeSet();
}