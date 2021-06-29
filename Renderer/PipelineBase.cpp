#include "PipelineBase.h"

std::vector<VkPipelineShaderStageCreateInfo> CPipelineBase::getDefaultShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule)
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

    return { VertShaderStageInfo, FragShaderStageInfo };
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

VkPipelineRasterizationStateCreateInfo CPipelineBase::getDefaultRasterizationStageInfo()
{
    // default clockwise, back culling, no depth bias
    VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
    RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.depthBiasEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

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
    DepthStencilInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
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

    _initDescriptorV();

    auto VertShaderCode = Common::readFileAsChar(_getVertShaderPathV());
    auto FragShaderCode = Common::readFileAsChar(_getFragShaderPathV());

    VkShaderModule VertShaderModule = Common::createShaderModule(m_Device, VertShaderCode);
    VkShaderModule FragShaderModule = Common::createShaderModule(m_Device, FragShaderCode);

    const auto& ShadeInfoSet = _getShadeStageInfoV(VertShaderModule, FragShaderModule);

    VkVertexInputBindingDescription Binding;
    std::vector<VkVertexInputAttributeDescription> AttributeSet;
    _getVertexInputInfoV(Binding, AttributeSet);
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.pVertexBindingDescriptions = &Binding;
    VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeSet.size());
    VertexInputInfo.pVertexAttributeDescriptions = AttributeSet.data();

    const auto& InputAssemblyInfo = _getInputAssemblyStageInfoV();

    VkPipelineViewportStateCreateInfo ViewportInfo = {};
    VkViewport Viewport;
    VkRect2D Scissor;
    _getViewportStageInfoV(vExtent, Viewport, Scissor);
    ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportInfo.viewportCount = 1;
    ViewportInfo.pViewports = &Viewport;
    ViewportInfo.scissorCount = 1;
    ViewportInfo.pScissors = &Scissor;

    const auto& RasterizationInfo = _getRasterizationStageInfoV();
    const auto& MultisampleInfo = _getMultisampleStageInfoV();
    const auto& DepthStencilInfo = _getDepthStencilInfoV();

    VkPipelineColorBlendAttachmentState BlendAttachement;
    _getColorBlendInfoV(BlendAttachement);
    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &BlendAttachement;

    const auto& EnabledDynamicSet = _getEnabledDynamicSetV();
    VkPipelineDynamicStateCreateInfo DynamicInfo = {};
    DynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicInfo.dynamicStateCount = static_cast<uint32_t>(EnabledDynamicSet.size());
    DynamicInfo.pDynamicStates = EnabledDynamicSet.data();

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
    PipelineInfo.stageCount = ShadeInfoSet.size();
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

    ck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_Pipeline));

    vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
}

void CPipelineBase::setImageNum(size_t vImageNum)
{
    if (vImageNum == m_ImageNum) return;
    m_ImageNum = vImageNum;
    m_Descriptor.createDescriptorSetSet(vImageNum);
    _createResourceV(vImageNum);
}

void CPipelineBase::destroy()
{
    m_ImageNum = 0;
    
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
    _initPushConstantV(vCommandBuffer);
}

void CPipelineBase::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
}

std::vector<VkPipelineShaderStageCreateInfo> CPipelineBase::_getShadeStageInfoV(VkShaderModule vVertModule, VkShaderModule vFragModule)
{
    return CPipelineBase::getDefaultShadeStageInfo(vVertModule, vFragModule);
}

VkPipelineInputAssemblyStateCreateInfo CPipelineBase::_getInputAssemblyStageInfoV()
{
    return CPipelineBase::getDefaultInputAssemblyStageInfo();
}

void CPipelineBase::_getViewportStageInfoV(VkExtent2D vExtent, VkViewport& voViewport, VkRect2D& voScissor)
{
    voViewport = {};
    voViewport.width = static_cast<float>(vExtent.width);
    voViewport.height = static_cast<float>(vExtent.height);
    voViewport.minDepth = 0.0f;
    voViewport.maxDepth = 1.0f;

    voScissor = {};
    voScissor.offset = { 0, 0 };
    voScissor.extent = vExtent;
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

void CPipelineBase::_getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment)
{
    // default disable blending
    voBlendAttachment = {};
    voBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    voBlendAttachment.blendEnable = VK_FALSE;
}

std::vector<VkDynamicState> CPipelineBase::_getEnabledDynamicSetV()
{
    return {};
}

std::vector<VkPushConstantRange> CPipelineBase::_getPushConstantRangeSetV()
{
    return CPipelineBase::getDefaultPushConstantRangeSet();
}