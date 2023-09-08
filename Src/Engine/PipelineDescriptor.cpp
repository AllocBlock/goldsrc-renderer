#include "PipelineDescriptor.h"
#include "Log.h"
#include "Environment.h"

bool CPipelineDescriptor::isValid() const
{
    return m_IsVertexInputValid;
}

std::vector<VkPipelineShaderStageCreateInfo> CPipelineDescriptor::getShadeStageInfo(VkShaderModule vVertModule,
    VkShaderModule vFragModule)
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

VkPipelineVertexInputStateCreateInfo CPipelineDescriptor::getVertexInputStageInfo() const
{
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.pVertexBindingDescriptions = &m_VertexInputBinding;
    VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexInputAttributeSet.size());
    VertexInputInfo.pVertexAttributeDescriptions = m_VertexInputAttributeSet.data();

    return VertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo CPipelineDescriptor::getInputAssemblyStageInfo() const
{
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = m_PrimitiveType;
    InputAssemblyInfo.primitiveRestartEnable = m_PrimitiveRestart ? VK_TRUE : VK_FALSE;

    return InputAssemblyInfo;
}

VkPipelineViewportStateCreateInfo CPipelineDescriptor::getViewportStageInfo(VkExtent2D vExtent)
{
    if (m_IsViewportSet)
        Log::log("Warning: viewport is reset and previous unsubmitted data will be invalid.");

    m_IsViewportSet = true;
    m_TempViewport = {};
    m_TempViewport.width = static_cast<float>(vExtent.width);
    m_TempViewport.height = static_cast<float>(vExtent.height);
    m_TempViewport.minDepth = 0.0f;
    m_TempViewport.maxDepth = 1.0f;

    m_TempScissor = {};
    m_TempScissor.offset = { 0, 0 };
    m_TempScissor.extent = vExtent;

    VkPipelineViewportStateCreateInfo ViewportInfo = {};
    ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportInfo.viewportCount = 1;
    ViewportInfo.pViewports = &m_TempViewport;
    ViewportInfo.scissorCount = 1;
    ViewportInfo.pScissors = &m_TempScissor;
    return ViewportInfo;
}

VkPipelineRasterizationStateCreateInfo CPipelineDescriptor::getRasterizationStageInfo() const
{
    VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
    RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.depthBiasEnable = VK_FALSE;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    RasterizerInfo.cullMode = m_RasterCullMode;
    RasterizerInfo.frontFace = m_RasterFrontFace;

    return RasterizerInfo;
}

VkPipelineMultisampleStateCreateInfo CPipelineDescriptor::getMultisampleStageInfo()
{
    // default no msaa
    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    return Multisampling;
}

VkPipelineDepthStencilStateCreateInfo CPipelineDescriptor::getDepthStencilInfo()
{
    // default enable depth test
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = m_EnableDepthTest ? VK_TRUE : VK_FALSE;
    DepthStencilInfo.depthWriteEnable = m_EnableDepthWrite ? VK_TRUE : VK_FALSE;
    DepthStencilInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

VkPipelineColorBlendStateCreateInfo CPipelineDescriptor::getColorBlendInfo()
{
    if (m_IsBlendAttachmentSet)
        Log::log("Warning: blend attachment is reset and previous unsubmitted data will be invalid.");
        
    m_IsBlendAttachmentSet = true;
    m_TempBlendAttachment = {};
    m_TempBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_TempBlendAttachment.blendEnable = m_EnableBlend ? VK_TRUE : VK_FALSE;
    if (m_EnableBlend)
    {
        m_TempBlendAttachment.srcColorBlendFactor = m_ColorBlendSrcFactor;
        m_TempBlendAttachment.dstColorBlendFactor = m_ColorBlendDstFactor;
        m_TempBlendAttachment.colorBlendOp = m_ColorBlendOp;
        m_TempBlendAttachment.srcAlphaBlendFactor = m_AlphaBlendSrcFactor;
        m_TempBlendAttachment.dstAlphaBlendFactor = m_AlphaBlendDstFactor;
        m_TempBlendAttachment.alphaBlendOp = m_AlphaBlendOp;
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &m_TempBlendAttachment;
    return ColorBlendInfo;
}

VkPipelineDynamicStateCreateInfo CPipelineDescriptor::getEnabledDynamicStateInfo()
{
    VkPipelineDynamicStateCreateInfo DynamicInfo = {};
    DynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicInfo.dynamicStateCount = static_cast<uint32_t>(m_DynamicStateSet.size());
    DynamicInfo.pDynamicStates = m_DynamicStateSet.data();
    return DynamicInfo;
}

std::vector<VkPushConstantRange> CPipelineDescriptor::getPushConstantRangeSet() const
{
    std::vector<VkPushConstantRange> PushConstantInfoSet(m_PushConstantSet.size());

    for (size_t i = 0; i < m_PushConstantSet.size(); ++i)
    {
        PushConstantInfoSet[i].stageFlags = m_PushConstantSet[i].first;
        PushConstantInfoSet[i].offset = 0;
        PushConstantInfoSet[i].size = m_PushConstantSet[i].second;
    }
    return PushConstantInfoSet;
}

void CPipelineDescriptor::setVertShaderPath(const std::filesystem::path& vShaderFileName)
{
    _ASSERTE(!vShaderFileName.empty());
    _ASSERTE(vShaderFileName.extension() == ".vert");
    m_VertShaderPath = Environment::findShader(vShaderFileName);
}

void CPipelineDescriptor::setFragShaderPath(const std::filesystem::path& vShaderFileName)
{
    _ASSERTE(!vShaderFileName.empty());
    _ASSERTE(vShaderFileName.extension() == ".frag");
    m_FragShaderPath = Environment::findShader(vShaderFileName);
}

void CPipelineDescriptor::setVertexInputInfo(VkVertexInputBindingDescription vBinding,
    const std::vector<VkVertexInputAttributeDescription>& vAttributeSet)
{
    m_VertexInputBinding = vBinding;
    m_VertexInputAttributeSet = vAttributeSet;
    m_IsVertexInputValid = true;
}

void CPipelineDescriptor::setInputAssembly(VkPrimitiveTopology vPrimitiveType, bool vPrimitiveRestart)
{
    m_PrimitiveType = vPrimitiveType;
    m_PrimitiveRestart= vPrimitiveRestart;
}

void CPipelineDescriptor::setDynamicStateSet(const std::vector<VkDynamicState>& vDynamicStateSet)
{
    if (m_IsDynamicStateSet)
        Log::log("Warning: blend attachment is reset and previous unsubmitted data will be invalid.");

    m_IsDynamicStateSet = true;
    m_DynamicStateSet = vDynamicStateSet;
}

void CPipelineDescriptor::addPushConstant(VkShaderStageFlags vStage, uint32_t vSize)
{
    m_PushConstantSet.push_back(std::make_pair(vStage , vSize));
}

void CPipelineDescriptor::clearPushConstant()
{
    m_PushConstantSet.clear();
}

void CPipelineDescriptor::setBlendMethod(EBlendFunction vFunction)
{
    switch (vFunction)
    {
    case EBlendFunction::NORMAL:
        {
            // result color = source color * source alpha + dst(old) color * (1 - source alpha)
            // result alpha = source alpha
            m_ColorBlendSrcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            m_ColorBlendDstFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            m_ColorBlendOp = VK_BLEND_OP_ADD;
            m_AlphaBlendSrcFactor = VK_BLEND_FACTOR_ONE;
            m_AlphaBlendDstFactor = VK_BLEND_FACTOR_ZERO;
            m_AlphaBlendOp = VK_BLEND_OP_ADD;
            break;
        }
    case EBlendFunction::ADDITIVE:
        {
            // additive: 
            // result color = (source color) * (source alpha) + old color
            // result alpha = whatever
            m_ColorBlendSrcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            m_ColorBlendDstFactor = VK_BLEND_FACTOR_ONE;
            m_AlphaBlendSrcFactor = m_AlphaBlendDstFactor = VK_BLEND_FACTOR_ONE;
            m_ColorBlendOp = m_AlphaBlendOp = VK_BLEND_OP_ADD;
            break;
        }
    default:
        throw std::runtime_error("Error: unknown blend function");
    }
}
