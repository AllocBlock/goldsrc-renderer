#include "Vulkan.h"
#include "Common.h"

#include <filesystem>

enum class EBlendFunction
{
    NORMAL,
    ADDITIVE
};

class CPipelineDescriptor
{
public:
    _DEFINE_GETTER(VertShaderPath, std::filesystem::path)
    _DEFINE_GETTER(FragShaderPath, std::filesystem::path)
    
    // TIPS: default backface culling
    _DEFINE_GETTER_SETTER(RasterCullMode, VkCullModeFlags)
    // TIPS: default counter clockwise
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

    bool isValid() const;
    
    static std::vector<VkPipelineShaderStageCreateInfo> getShadeStageInfo(VkShaderModule vVertModule, VkShaderModule vFragModule);
    VkPipelineVertexInputStateCreateInfo getVertexInputStageInfo() const;
    VkPipelineInputAssemblyStateCreateInfo getInputAssemblyStageInfo() const;
    VkPipelineViewportStateCreateInfo getViewportStageInfo(VkExtent2D vExtent);
    VkPipelineRasterizationStateCreateInfo getRasterizationStageInfo() const; // TIPS: default counter clockwise and backface culling
    static VkPipelineMultisampleStateCreateInfo getMultisampleStageInfo();
    VkPipelineDepthStencilStateCreateInfo getDepthStencilInfo();
    VkPipelineColorBlendStateCreateInfo getColorBlendInfo();
    VkPipelineDynamicStateCreateInfo getEnabledDynamicStateInfo();
    std::vector<VkPushConstantRange> getPushConstantRangeSet() const;

    void setVertShaderPath(const std::filesystem::path& vShaderFileName);
    void setFragShaderPath(const std::filesystem::path& vShaderFileName);
    void setVertexInputInfo(VkVertexInputBindingDescription vBinding, const std::vector<VkVertexInputAttributeDescription>& vAttributeSet);

    template <typename PointData_t>
    void setVertexInputInfo()
    {
        setVertexInputInfo(PointData_t::getBindingDescription(), PointData_t::getAttributeDescriptionSet());
    }

    // TIPS: default it's triangle list and no primitive restart
    void setInputAssembly(VkPrimitiveTopology vPrimitiveType, bool vPrimitiveRestart = false);
    void setDynamicStateSet(const std::vector<VkDynamicState>& vDynamicStateSet);
    void addPushConstant(VkShaderStageFlags vStage, uint32_t vSize);

    template <typename SPushConstant_t>
    void addPushConstant(VkShaderStageFlags vStage)
    {
        m_PushConstantSet.push_back(std::make_pair(vStage, sizeof(SPushConstant_t)));
    }

    void clearPushConstant();
    void setBlendMethod(EBlendFunction vFunction);

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
    VkFrontFace m_RasterFrontFace = VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;

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

    std::vector<std::pair<VkShaderStageFlags, uint32_t>> m_PushConstantSet;

    VkPipelineRasterizationStateCreateInfo m_RasterizationStageInfo;
    VkPipelineMultisampleStateCreateInfo m_MultisampleStageInfo;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStageInfo;
    VkPipelineColorBlendAttachmentState m_BlendAttachmentStageInfo;
    std::vector<VkDynamicState> m_DynamicStageInfo;
};
