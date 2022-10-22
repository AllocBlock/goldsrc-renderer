#include "Vulkan.h"
#include "Common.h"
#include "Log.h"

#include <filesystem>

class CPipelineDescriptor
{
public:
    _DEFINE_GETTER_SETTER(VertShaderPath, std::filesystem::path)
    _DEFINE_GETTER_SETTER(FragShaderPath, std::filesystem::path)
    
    // TIPS: default backface culling
    _DEFINE_GETTER_SETTER(RasterCullMode, VkCullModeFlags)
    // TIPS: default clockwise
    _DEFINE_GETTER_SETTER(RasterFrontFace, VkFrontFace)

    // TIPS: default on
    _DEFINE_GETTER_SETTER(EnableDepthTest, bool)
    // TIPS: default on
    _DEFINE_GETTER_SETTER(EnableDepthWrite, bool)
        
    // TIPS: default off
    _DEFINE_GETTER_SETTER(EnableBlend, bool)
    // TIPS: default VK_BLEND_FACTOR_SRC_ALPHA
    _DEFINE_GETTER_SETTER(ColorBlendSrcFactor, VkBlendFactor)
    // TIPS: default VK_BLEND_FACTOR_ONE
    _DEFINE_GETTER_SETTER(ColorBlendDstFactor, VkBlendFactor)
    // TIPS: default VK_BLEND_OP_ADD
    _DEFINE_GETTER_SETTER(ColorBlendOp, VkBlendOp)
    // TIPS: default VK_BLEND_FACTOR_ONE
    _DEFINE_GETTER_SETTER(AlphaBlendSrcFactor, VkBlendFactor)
    // TIPS: default VK_BLEND_FACTOR_ONE
    _DEFINE_GETTER_SETTER(AlphaBlendDstFactor, VkBlendFactor)
    // TIPS: default VK_BLEND_OP_ADD
    _DEFINE_GETTER_SETTER(AlphaBlendOp, VkBlendOp)

    _DEFINE_GETTER(DynamicStateSet, std::vector<VkDynamicState>)

    bool isValid() const 
    {
        return m_IsVertexInputValid;
    }

    // TODO: how to custom?
    static std::vector<VkPipelineShaderStageCreateInfo> getShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule)
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
    
    VkPipelineVertexInputStateCreateInfo getVertexInputStageInfo() const
    {
        VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputInfo.pVertexBindingDescriptions = &m_VertexInputBinding;
        VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexInputAttributeSet.size());
        VertexInputInfo.pVertexAttributeDescriptions = m_VertexInputAttributeSet.data();

        return VertexInputInfo;
    }

    VkPipelineInputAssemblyStateCreateInfo getInputAssemblyStageInfo() const
    {
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
        InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyInfo.topology = m_PrimitiveType;
        InputAssemblyInfo.primitiveRestartEnable = m_PrimitiveRestart ? VK_TRUE : VK_FALSE;

        return InputAssemblyInfo;
    }

    VkPipelineViewportStateCreateInfo getViewportStageInfo(VkExtent2D vExtent)
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

    // TIPS: default clockwise and backface culling
    VkPipelineRasterizationStateCreateInfo getRasterizationStageInfo() const
    {
        // default clockwise, back culling, no depth bias
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

    static VkPipelineMultisampleStateCreateInfo getMultisampleStageInfo()
    {
        // default no msaa
        VkPipelineMultisampleStateCreateInfo Multisampling = {};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        return Multisampling;
    }

    VkPipelineDepthStencilStateCreateInfo getDepthStencilInfo()
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

    VkPipelineColorBlendStateCreateInfo getColorBlendInfo()
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

    VkPipelineDynamicStateCreateInfo getEnabledDynamicStateInfo()
    {
        VkPipelineDynamicStateCreateInfo DynamicInfo = {};
        DynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicInfo.dynamicStateCount = static_cast<uint32_t>(m_DynamicStateSet.size());
        DynamicInfo.pDynamicStates = m_DynamicStateSet.data();
        return DynamicInfo;
    }

    std::vector<VkPushConstantRange> getPushConstantRangeSet() const
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

    void setVertexInputInfo(VkVertexInputBindingDescription vBinding, const std::vector<VkVertexInputAttributeDescription>& vAttributeSet)
    {
        m_VertexInputBinding = vBinding;
        m_VertexInputAttributeSet = vAttributeSet;
        m_IsVertexInputValid = true;
    }

    template <typename PointData_t>
    void setVertexInputInfo()
    {
        setVertexInputInfo(PointData_t::getBindingDescription(), PointData_t::getAttributeDescriptionSet());
    }

    // TIPS: default it's triangle list and no primitive restart
    void setInputAssembly(VkPrimitiveTopology vPrimitiveType, bool vPrimitiveRestart = false)
    {
        m_PrimitiveType = vPrimitiveType;
        m_PrimitiveRestart= vPrimitiveRestart;
    }
    
    void setDynamicStateSet(const std::vector<VkDynamicState>& vDynamicStateSet)
    {
        if (m_IsDynamicStateSet)
            Log::log("Warning: blend attachment is reset and previous unsubmitted data will be invalid.");

        m_IsDynamicStateSet = true;
        m_DynamicStateSet = vDynamicStateSet;
    }

    void addPushConstant(VkShaderStageFlags vStage, VkDeviceSize vSize)
    {
        m_PushConstantSet.push_back(std::make_pair(vStage , vSize));
    }

    template <typename SPushConstant_t>
    void addPushConstant(VkShaderStageFlags vStage)
    {
        m_PushConstantSet.push_back(std::make_pair(vStage , sizeof(SPushConstant_t)));
    }

    void clearPushConstant()
    {
        m_PushConstantSet.clear();
    }

    //virtual void _initDescriptorV() = 0;
    //virtual void _initPushConstantV(VkCommandBuffer vCommandBuffer);
    //virtual std::vector<VkPipelineShaderStageCreateInfo> _getShadeStageInfoV(VkShaderModule vVertModule, VkShaderModule vFragModule);
    //virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) = 0;
    //virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV();
    //virtual void _getViewportStageInfoV(VkExtent2D vExtent, VkViewport& voViewport, VkRect2D& voScissor);
    //virtual VkPipelineRasterizationStateCreateInfo _getRasterizationStageInfoV();
    //virtual VkPipelineMultisampleStateCreateInfo _getMultisampleStageInfoV();
    //virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV();
    //virtual void _getColorBlendInfoV(VkPipelineColorBlendAttachmentState& voBlendAttachment);
    //virtual std::vector<VkDynamicState> _getEnabledDynamicSetV();
    //virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV();

private:
    std::filesystem::path m_VertShaderPath;
    std::filesystem::path m_FragShaderPath;

    bool m_IsVertexInputValid = false;
    VkVertexInputBindingDescription m_VertexInputBinding;
    std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributeSet;

    VkPrimitiveTopology m_PrimitiveType = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool m_PrimitiveRestart = false;

    bool m_IsViewportSet = false;
    VkViewport m_TempViewport;
    VkRect2D m_TempScissor;

    VkCullModeFlags m_RasterCullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
    VkFrontFace m_RasterFrontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;

    bool m_EnableDepthWrite = true;
    bool m_EnableDepthTest = true;
    
    bool m_IsBlendAttachmentSet = false;
    VkPipelineColorBlendAttachmentState m_TempBlendAttachment;
    bool m_EnableBlend = false;
    VkBlendFactor m_ColorBlendSrcFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor m_ColorBlendDstFactor = VK_BLEND_FACTOR_ONE;
    VkBlendOp m_ColorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor m_AlphaBlendSrcFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor m_AlphaBlendDstFactor = VK_BLEND_FACTOR_ONE;
    VkBlendOp m_AlphaBlendOp = VK_BLEND_OP_ADD;

    bool m_IsDynamicStateSet = false;
    std::vector<VkDynamicState> m_DynamicStateSet;

    std::vector<std::pair<VkShaderStageFlags, VkDeviceSize>> m_PushConstantSet;

    VkPipelineRasterizationStateCreateInfo m_RasterizationStageInfo;
    VkPipelineMultisampleStateCreateInfo m_MultisampleStageInfo;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStageInfo;
    VkPipelineColorBlendAttachmentState m_BlendAttachmentStageInfo;
    std::vector<VkDynamicState> m_DynamicStageInfo;
};
