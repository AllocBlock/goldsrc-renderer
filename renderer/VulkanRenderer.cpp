#include "VulkanRenderer.h"
#include "IOLog.h"
#include "Common.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

CVulkanRenderer::CVulkanRenderer()
    : m_pCamera(std::make_shared<CCamera>())
{
}

void CVulkanRenderer::init(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicsFamilyIndex, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    m_Instance = vInstance;
    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;
    m_GraphicsQueueIndex = vGraphicsFamilyIndex;
    m_ImageFormat = vImageFormat;
    m_Extent = vExtent;
    m_ImageViews = vImageViews;
    vkGetDeviceQueue(m_Device, m_GraphicsQueueIndex, 0, &m_GraphicsQueue);
    __createRenderPass();
    __createDescriptorSetLayout();
    __createCommandPool();
    __createTextureSampler();
    __createRecreateResources();
}

void CVulkanRenderer::__createRecreateResources()
{
    __createGraphicsPipeline(); // extent
    __createDepthResources(); // extent
    __createFramebuffers(); // imageview, extent
    __createUniformBuffers(); // imageview
    __createDescriptorPool(); // imageview
    __createDescriptorSets(); // imageview
    __createSceneResources();
}

void CVulkanRenderer::__destroyRecreateResources()
{
    vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    vkDestroyImage(m_Device, m_DepthImage, nullptr);
    vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);
    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);

    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    for (size_t i = 0; i < m_ImageViews.size(); ++i)
    {
        vkDestroyBuffer(m_Device, m_VertUniformBuffers[i], nullptr);
        vkFreeMemory(m_Device, m_VertUniformBufferMemories[i], nullptr);
        vkDestroyBuffer(m_Device, m_FragUniformBuffers[i], nullptr);
        vkFreeMemory(m_Device, m_FragUniformBufferMemories[i], nullptr);
    }
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

    __destroySceneResources();
}

void CVulkanRenderer::__createSceneResources()
{
    __createTextureImages(); // scene
    __createTextureImageViews(); // scene
    __createLightmapImages(); // scene
    __createLightmapImageViews(); // scene
    __updateDescriptorSets();
    __createVertexBuffer(); // scene
    __createIndexBuffer(); // scene
    __createCommandBuffers(); // scene
}

void CVulkanRenderer::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImages.size(); ++i)
    {
        vkDestroyImageView(m_Device, m_TextureImageViews[i], nullptr);
        vkDestroyImage(m_Device, m_TextureImages[i], nullptr);
        vkFreeMemory(m_Device, m_TextureImageMemories[i], nullptr);
    }

    for (size_t i = 0; i < m_LightmapImages.size(); ++i)
    {
        vkDestroyImageView(m_Device, m_LightmapImageViews[i], nullptr);
        vkDestroyImage(m_Device, m_LightmapImages[i], nullptr);
        vkFreeMemory(m_Device, m_LightmapImageMemories[i], nullptr);
    }

    vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
    vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
    vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
}

void CVulkanRenderer::destroy()
{
    __destroyRecreateResources();

    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
}

void CVulkanRenderer::loadScene(const SScene& vScene)
{
     m_Scene = vScene;
     m_ObjectDataPositions.resize(m_Scene.Objects.size());
     if (m_Scene.BspTree.Nodes.empty())
         m_RenderMethod = ERenderMethod::DEFAULT;

     size_t IndexOffset = 0;
     size_t VertexOffset = 0;
     for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
     {
         std::shared_ptr<S3DObject> pObject = m_Scene.Objects[i];
         if (pObject->Type == E3DObjectType::INDEXED_TRIAGNLE_LIST)
         { 
             m_ObjectDataPositions[i].Offset = IndexOffset;
             m_ObjectDataPositions[i].Size = pObject->Indices.size();
             IndexOffset += m_ObjectDataPositions[i].Size;
         }
         else if (pObject->Type == E3DObjectType::TRIAGNLE_LIST)
         {
             m_ObjectDataPositions[i].Offset = VertexOffset;
             m_ObjectDataPositions[i].Size = pObject->Vertices.size();
             VertexOffset += m_ObjectDataPositions[i].Size;
         }
         else
             throw std::runtime_error(u8"物体类型错误");
     }

     m_AreObjectsVisable.clear();
     m_AreObjectsVisable.resize(m_Scene.Objects.size(), false);
     m_VisableObjectNum = 0;

     vkDeviceWaitIdle(m_Device);
     __destroySceneResources();
     __createSceneResources();
}

VkCommandBuffer CVulkanRenderer::requestCommandBuffer(uint32_t vImageIndex)
{
    _ASSERTE(vImageIndex >= 0 && vImageIndex < m_CommandBuffers.size());
    // TODO: not finished, and to to consider dynamic, multiple command buffer, which should not be handled using static vector
    bool RerecordCommand = false;
    if (m_RenderMethod == ERenderMethod::BSP || m_EnableCulling || m_RerecordCommand > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommand > 0) --m_RerecordCommand;
    }
    if (RerecordCommand)
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        ck(vkBeginCommandBuffer(m_CommandBuffers[vImageIndex], &CommandBufferBeginInfo));

        std::array<VkClearValue, 2> ClearValues = {};
        ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = m_RenderPass;
        RenderPassBeginInfo.framebuffer = m_Framebuffers[vImageIndex];
        RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        RenderPassBeginInfo.renderArea.extent = m_Extent;
        RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
        RenderPassBeginInfo.pClearValues = ClearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffers[vImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_CommandBuffers[vImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

        VkBuffer VertexBuffers[] = { m_VertexBuffer };
        VkDeviceSize Offsets[] = { 0 };
        if (m_VertexBuffer != VK_NULL_HANDLE)
            vkCmdBindVertexBuffers(m_CommandBuffers[vImageIndex], 0, 1, VertexBuffers, Offsets);
        if (m_IndexBuffer != VK_NULL_HANDLE)
            vkCmdBindIndexBuffer(m_CommandBuffers[vImageIndex], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(m_CommandBuffers[vImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[vImageIndex], 0, nullptr);
        
        if (m_VertexBuffer != VK_NULL_HANDLE || m_IndexBuffer != VK_NULL_HANDLE)
        {
            __calculateVisiableObjects();
            if (m_RenderMethod == ERenderMethod::BSP)
                __renderByBspTree(vImageIndex);
            else
            {
                for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
                {
                    if (m_AreObjectsVisable[i])
                        __recordObjectRenderCommand(vImageIndex, i);
                }
            }
        }

        vkCmdEndRenderPass(m_CommandBuffers[vImageIndex]);
        ck(vkEndCommandBuffer(m_CommandBuffers[vImageIndex]));
    }
    return m_CommandBuffers[vImageIndex];
}

void CVulkanRenderer::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeList.clear();
    if (m_Scene.BspTree.Nodes.empty()) throw "场景不含BSP数据";
    __renderTreeNode(vImageIndex, 0);
}

void CVulkanRenderer::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    if (vNodeIndex >= m_Scene.BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t ObjectIndex = vNodeIndex - m_Scene.BspTree.NodeNum;
        if (ObjectIndex > 0 && m_AreObjectsVisable[ObjectIndex])
        {
            m_RenderNodeList.emplace_back(ObjectIndex);
            __recordObjectRenderCommand(vImageIndex, ObjectIndex);
        }
    }
    else
    {
        const SBspTreeNode& Node = m_Scene.BspTree.Nodes[vNodeIndex];
        glm::vec3 CameraPos = m_pCamera->getPos();
        if (Node.isPointFrontOfPlane(CameraPos))
        {
            __renderTreeNode(vImageIndex, Node.Back.value());
            __renderTreeNode(vImageIndex, Node.Front.value());
        }
        else
        {
            __renderTreeNode(vImageIndex, Node.Front.value());
            __renderTreeNode(vImageIndex, Node.Back.value());
        }
    }
}

void CVulkanRenderer::rerecordCommand()
{
    m_RerecordCommand += m_CommandBuffers.size();
}

std::shared_ptr<CCamera> CVulkanRenderer::getCamera()
{
    return m_pCamera;
}

void CVulkanRenderer::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    std::shared_ptr<S3DObject> pObject = m_Scene.Objects[vObjectIndex];
    SObjectDataPosition DataPosition = m_ObjectDataPositions[vObjectIndex];
    //SPushConstant PushConstant = {};
    vkCmdSetDepthBias(m_CommandBuffers[vImageIndex], static_cast<float>(vObjectIndex) / m_Scene.Objects.size(), 0, 0);
    //vkCmdPushConstants(m_CommandBuffers[vImageIndex], m_PipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SPushConstant), &PushConstant);
    if (pObject->Type == E3DObjectType::INDEXED_TRIAGNLE_LIST)
        vkCmdDrawIndexed(m_CommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0, 0);
    else if (pObject->Type == E3DObjectType::TRIAGNLE_LIST)
        vkCmdDraw(m_CommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0);
    else if (pObject->Type == E3DObjectType::TRIAGNLE_STRIP_LIST)
        vkCmdDraw(m_CommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0);
    else
        throw std::runtime_error(u8"物体类型错误");
}

void CVulkanRenderer::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_ImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // not present but next render pass

    VkAttachmentDescription DepthAttachment = {};
    DepthAttachment.format = __findDepthFormat();
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency SubpassDependency = {};
    SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependency.dstSubpass = 0;
    SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.srcAccessMask = 0;
    SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

    std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &SubpassDesc;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &SubpassDependency;

    ck(vkCreateRenderPass(m_Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CVulkanRenderer::__createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding UboVertBinding = {};
    UboVertBinding.binding = 0;
    UboVertBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboVertBinding.descriptorCount = 1;
    UboVertBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding UboFragBinding = {};
    UboFragBinding.binding = 1;
    UboFragBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboFragBinding.descriptorCount = 1;
    UboFragBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding SamplerBinding = {};
    SamplerBinding.binding = 2;
    SamplerBinding.descriptorCount = 1;
    SamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    SamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    SamplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding TextureBinding = {};
    TextureBinding.binding = 3;
    TextureBinding.descriptorCount = m_MaxTextureNum;
    TextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    TextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    TextureBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding LightmapBinding = {};
    LightmapBinding.binding = 4;
    LightmapBinding.descriptorCount = m_MaxLightmapNum;
    LightmapBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    LightmapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    LightmapBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> Bindings = 
    {
        UboVertBinding,
        UboFragBinding,
        SamplerBinding,
        TextureBinding,
        LightmapBinding
    };
    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void CVulkanRenderer::__createGraphicsPipeline()
{
    auto VertShaderCode = __readFile(m_VertShaderPath);
    auto FragShaderCode = __readFile(m_FragShaderPath);

    VkShaderModule VertShaderModule = __createShaderModule(VertShaderCode);
    VkShaderModule FragShaderModule = __createShaderModule(FragShaderCode);

    VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
    VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VertShaderStageInfo.module = VertShaderModule;
    VertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
    FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    FragShaderStageInfo.module = FragShaderModule;
    FragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

    auto BindingDescription = SPointData::getBindingDescription();
    auto AttributeDescriptions = SPointData::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
    VertexInputInfo.pVertexBindingDescriptions = &BindingDescription;
    VertexInputInfo.pVertexAttributeDescriptions = AttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
    InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyInfo.topology = m_DefaultPrimitiveToplogy;
    InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport Viewport = {};
    Viewport.width = static_cast<float>(m_Extent.width);
    Viewport.height = static_cast<float>(m_Extent.height);
    Viewport.minDepth = 0.0f;
    Viewport.maxDepth = 1.0f;

    VkRect2D Scissor = {};
    Scissor.offset = { 0, 0 };
    Scissor.extent = m_Extent;

    VkPipelineViewportStateCreateInfo ViewportStateInfo = {};
    ViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateInfo.viewportCount = 1;
    ViewportStateInfo.pViewports = &Viewport;
    ViewportStateInfo.scissorCount = 1;
    ViewportStateInfo.pScissors = &Scissor;

    VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
    RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerInfo.depthClampEnable = VK_FALSE;
    RasterizerInfo.depthBiasEnable = VK_TRUE;
    RasterizerInfo.depthBiasConstantFactor = 0.0;
    RasterizerInfo.depthBiasSlopeFactor = 1.0;
    RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizerInfo.lineWidth = 1.0f;
    /*RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;*/
    //RasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo Multisampling = {};
    Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    Multisampling.minSampleShading = 1.0f; // Optional
    Multisampling.pSampleMask = nullptr; // Optional
    Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    Multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo ColorBlending = {};
    ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable = VK_FALSE;
    ColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    ColorBlending.attachmentCount = 1;
    ColorBlending.pAttachments = &ColorBlendAttachment;
    ColorBlending.blendConstants[0] = 0.0f; // Optional
    ColorBlending.blendConstants[1] = 0.0f; // Optional
    ColorBlending.blendConstants[2] = 0.0f; // Optional
    ColorBlending.blendConstants[3] = 0.0f; // Optional

    std::vector<VkDynamicState> EnabledDynamicStates = 
    {
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = static_cast<uint32_t>(EnabledDynamicStates.size());
    DynamicState.pDynamicStates = EnabledDynamicStates.data();

    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = 1;
    PipelineLayoutInfo.pPushConstantRanges = &PushConstantInfo;

    ck(vkCreatePipelineLayout(m_Device, &PipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = ShaderStages;
    PipelineInfo.pVertexInputState = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
    PipelineInfo.pViewportState = &ViewportStateInfo;
    PipelineInfo.pRasterizationState = &RasterizerInfo;
    PipelineInfo.pMultisampleState = &Multisampling;
    PipelineInfo.pDepthStencilState = &DepthStencilInfo;
    PipelineInfo.pColorBlendState = &ColorBlending;
    PipelineInfo.pDynamicState = &DynamicState;
    PipelineInfo.layout = m_PipelineLayout;
    PipelineInfo.renderPass = m_RenderPass;
    PipelineInfo.subpass = 0;

    ck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_Pipeline));

    vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
}

void CVulkanRenderer::__createCommandPool()
{
    VkCommandPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PoolInfo.queueFamilyIndex = m_GraphicsQueueIndex;

    ck(vkCreateCommandPool(m_Device, &PoolInfo, nullptr, &m_CommandPool));
}

void CVulkanRenderer::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();
    __createImage(m_Extent.width, m_Extent.height, DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = Common::createImageView(m_Device, m_DepthImage, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    __transitionImageLayout(m_DepthImage, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void CVulkanRenderer::__createFramebuffers()
{
    m_Framebuffers.resize(m_ImageViews.size());
    for (size_t i = 0; i < m_ImageViews.size(); ++i)
    {
        std::array<VkImageView, 2> Attachments =
        {
            m_ImageViews[i],
            m_DepthImageView
        };

        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = m_RenderPass;
        FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        FramebufferInfo.pAttachments = Attachments.data();
        FramebufferInfo.width = m_Extent.width;
        FramebufferInfo.height = m_Extent.height;
        FramebufferInfo.layers = 1;

        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_Framebuffers[i]));
    }
}

void CVulkanRenderer::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImages.resize(NumTexture);
        m_TextureImageMemories.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            std::shared_ptr<CIOImage> pImage = m_Scene.TexImages[i];
            __createImageFromIOImage(pImage, m_TextureImages[i], m_TextureImageMemories[i]);
        }
    }
}

void CVulkanRenderer::__createTextureImageViews()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageViews.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
            m_TextureImageViews[i] = Common::createImageView(m_Device, m_TextureImages[i], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CVulkanRenderer::__createLightmapImages()
{
    size_t NumLightmap = __getActualLightmapNum();
    if (NumLightmap > 0)
    {
        m_LightmapImages.resize(NumLightmap);
        m_LightmapImageMemories.resize(NumLightmap);
        for (size_t i = 0; i < NumLightmap; ++i)
        {
            std::shared_ptr<CIOImage> pImage = m_Scene.LightmapImages[i];
            __createImageFromIOImage(pImage, m_LightmapImages[i], m_LightmapImageMemories[i]);
        }
    }
}

void CVulkanRenderer::__createLightmapImageViews()
{
    size_t NumLightmap = __getActualLightmapNum();
    if (NumLightmap > 0)
    {
        m_LightmapImageViews.resize(NumLightmap);
        for (size_t i = 0; i < NumLightmap; ++i)
            m_LightmapImageViews[i] = Common::createImageView(m_Device, m_LightmapImages[i], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CVulkanRenderer::__createTextureSampler()
{
    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);

    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    ck(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));
}

void CVulkanRenderer::__createVertexBuffer()
{
    size_t NumVertex = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
        NumVertex += pObject->Vertices.size();
    if (NumVertex == 0)
    {
        GlobalLogger::logStream() << u8"没有顶点数据，跳过索引缓存创建";
        return;
    }

    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
    {
        std::vector<SPointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SPointData) * pObject->Vertices.size();
        memcpy(reinterpret_cast<char*>(pData)+ Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

    __copyBuffer(StagingBuffer, m_VertexBuffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createIndexBuffer()
{
    size_t NumIndex = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
        NumIndex += pObject->Indices.size();

    if (NumIndex == 0)
    {
        GlobalLogger::logStream() << u8"没有索引数据，跳过索引缓存创建";
        return;
    }

    VkDeviceSize BufferSize = sizeof(uint32_t) * NumIndex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
    {
        size_t IndexOffset = Offset / sizeof(uint32_t);
        std::vector<uint32_t> Indices = pObject->Indices;
        for (uint32_t& Index : Indices)
            Index += IndexOffset;
        size_t SubBufferSize = sizeof(uint32_t) * Indices.size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, Indices.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    __copyBuffer(StagingBuffer, m_IndexBuffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createUniformBuffers()
{
    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    size_t NumSwapchainImage = m_ImageViews.size();
    m_VertUniformBuffers.resize(NumSwapchainImage);
    m_VertUniformBufferMemories.resize(NumSwapchainImage);
    m_FragUniformBuffers.resize(NumSwapchainImage);
    m_FragUniformBufferMemories.resize(NumSwapchainImage);

    for (size_t i = 0; i < m_ImageViews.size(); ++i)
    {
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBuffers[i], m_VertUniformBufferMemories[i]);
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBuffers[i], m_FragUniformBufferMemories[i]);
    }
}

void CVulkanRenderer::__createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> PoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_ImageViews.size()) },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_ImageViews.size()) },
        { VK_DESCRIPTOR_TYPE_SAMPLER, static_cast<uint32_t>(m_ImageViews.size()) },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(m_ImageViews.size() * m_MaxTextureNum) },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(m_ImageViews.size() * m_MaxLightmapNum) }
    };

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(m_ImageViews.size());

    ck(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CVulkanRenderer::__createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> Layouts(m_ImageViews.size(), m_DescriptorSetLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_ImageViews.size());
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_DescriptorSets.resize(m_ImageViews.size());
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSets.data()));
}

void CVulkanRenderer::__updateDescriptorSets()
{
    for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBuffers[i];
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBuffers[i];
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;

        std::vector<VkWriteDescriptorSet> DescriptorWrites;

        VkWriteDescriptorSet VertBufferDescriptorWrite = {};
        VertBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VertBufferDescriptorWrite.dstSet = m_DescriptorSets[i];
        VertBufferDescriptorWrite.dstBinding = 0;
        VertBufferDescriptorWrite.dstArrayElement = 0;
        VertBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        VertBufferDescriptorWrite.descriptorCount = 1;
        VertBufferDescriptorWrite.pBufferInfo = &VertBufferInfo;
        DescriptorWrites.emplace_back(VertBufferDescriptorWrite);

        VkWriteDescriptorSet FragBufferDescriptorWrite = {};
        FragBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FragBufferDescriptorWrite.dstSet = m_DescriptorSets[i];
        FragBufferDescriptorWrite.dstBinding = 1;
        FragBufferDescriptorWrite.dstArrayElement = 0;
        FragBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        FragBufferDescriptorWrite.descriptorCount = 1;
        FragBufferDescriptorWrite.pBufferInfo = &FragBufferInfo;
        DescriptorWrites.emplace_back(FragBufferDescriptorWrite);

        VkWriteDescriptorSet SamplerDescriptorWrite = {};
        SamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        SamplerDescriptorWrite.dstSet = m_DescriptorSets[i];
        SamplerDescriptorWrite.dstBinding = 2;
        SamplerDescriptorWrite.dstArrayElement = 0;
        SamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        SamplerDescriptorWrite.descriptorCount = 1;
        SamplerDescriptorWrite.pImageInfo = &SamplerInfo;
        DescriptorWrites.emplace_back(SamplerDescriptorWrite);

        const size_t NumTexture = __getActualTextureNum();
        std::vector<VkDescriptorImageInfo> TexImageInfos;
        if (NumTexture > 0)
        {
            TexImageInfos.resize(m_MaxTextureNum);
            for (size_t i = 0; i < m_MaxTextureNum; ++i)
            {
                // for unused element, fill like the first one (weird method but avoid validationwarning)
                if (i >= NumTexture)
                {
                    TexImageInfos[i] = TexImageInfos[0];
                }
                else
                {
                    TexImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfos[i].imageView = m_TextureImageViews[i];
                    TexImageInfos[i].sampler = VK_NULL_HANDLE;
                }
            }

            VkWriteDescriptorSet TexturesDescriptorWrite = {};
            TexturesDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            TexturesDescriptorWrite.dstSet = m_DescriptorSets[i];
            TexturesDescriptorWrite.dstBinding = 3;
            TexturesDescriptorWrite.dstArrayElement = 0;
            TexturesDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            TexturesDescriptorWrite.descriptorCount = static_cast<uint32_t>(TexImageInfos.size());
            TexturesDescriptorWrite.pImageInfo = TexImageInfos.data();
            DescriptorWrites.emplace_back(TexturesDescriptorWrite);
        }
        
        const size_t NumLightmap = __getActualLightmapNum();
        std::vector<VkDescriptorImageInfo> LightmapImageInfos;
        if (NumLightmap > 0)
        {
            LightmapImageInfos.resize(m_MaxLightmapNum);
            for (size_t i = 0; i < m_MaxLightmapNum; ++i)
            {
                // for unused element, fill like the first one (weird method but avoid validationwarning)
                if (i >= NumLightmap)
                {
                    LightmapImageInfos[i] = LightmapImageInfos[0];
                }
                else
                {
                    LightmapImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    LightmapImageInfos[i].imageView = m_LightmapImageViews[i];
                    LightmapImageInfos[i].sampler = VK_NULL_HANDLE;
                }
            }

            VkWriteDescriptorSet LightmapsDescriptorWrite = {};
            LightmapsDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            LightmapsDescriptorWrite.dstSet = m_DescriptorSets[i];
            LightmapsDescriptorWrite.dstBinding = 4;
            LightmapsDescriptorWrite.dstArrayElement = 0;
            LightmapsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            LightmapsDescriptorWrite.descriptorCount = static_cast<uint32_t>(LightmapImageInfos.size());
            LightmapsDescriptorWrite.pImageInfo = LightmapImageInfos.data();
            DescriptorWrites.emplace_back(LightmapsDescriptorWrite);
        }

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }
}

void CVulkanRenderer::__createCommandBuffers()
{
    m_CommandBuffers.resize(m_Framebuffers.size());

    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.commandPool = m_CommandPool;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    ck(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocInfo, m_CommandBuffers.data()));

    rerecordCommand();
}

std::vector<char> CVulkanRenderer::__readFile(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::ate | std::ios::binary);

    if (!File.is_open()) 
        throw std::runtime_error(u8"读取文件失败：" + vFilePath.u8string());

    size_t FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> Buffer(FileSize);
    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();

    return Buffer;
}

std::vector<SPointData> CVulkanRenderer::__readPointData(std::shared_ptr<S3DObject> vpObject) const
{
    size_t NumPoint = vpObject->Vertices.size();
    _ASSERTE(NumPoint == vpObject->Colors.size());
    _ASSERTE(NumPoint == vpObject->Normals.size());
    _ASSERTE(NumPoint == vpObject->TexCoords.size());
    _ASSERTE(NumPoint == vpObject->LightmapCoords.size());
    _ASSERTE(NumPoint == vpObject->TexIndices.size());
    _ASSERTE(NumPoint == vpObject->LightmapIndices.size());

    std::vector<SPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = vpObject->Vertices[i];
        PointData[i].Color = vpObject->Colors[i];
        PointData[i].Normal = vpObject->Normals[i];
        PointData[i].TexCoord = vpObject->TexCoords[i];
        PointData[i].LightmapCoord = vpObject->LightmapCoords[i];
        PointData[i].TexIndex = vpObject->TexIndices[i];
        PointData[i].LightmapIndex = vpObject->LightmapIndices[i];
    }
    return PointData;
}

VkFormat CVulkanRenderer::__findDepthFormat()
{
    return __findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CVulkanRenderer::__findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
{
    for (VkFormat Format : vCandidates)
    {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, Format, &Props);

        if (vTiling == VK_IMAGE_TILING_LINEAR && 
            (Props.linearTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
        else if (vTiling == VK_IMAGE_TILING_OPTIMAL && 
            (Props.optimalTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
    }

    throw std::runtime_error(u8"未找到适配的vulkan格式");
}

VkShaderModule CVulkanRenderer::__createShaderModule(const std::vector<char>& vShaderCode)
{
    VkShaderModuleCreateInfo ShaderModuleInfo = {};
    ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleInfo.codeSize = vShaderCode.size();
    ShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

    VkShaderModule ShaderModule;
    ck(vkCreateShaderModule(m_Device, &ShaderModuleInfo, nullptr, &ShaderModule));
    return ShaderModule;
}

void CVulkanRenderer::__createImage(uint32_t vWidth, uint32_t vHeight, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = vWidth;
    ImageInfo.extent.height = vHeight;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = vFormat;
    ImageInfo.tiling = vTiling;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = vUsage;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateImage(m_Device, &ImageInfo, nullptr, &voImage));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(m_Device, voImage, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voImageMemory));

    ck(vkBindImageMemory(m_Device, voImage, voImageMemory, 0));
}

uint32_t CVulkanRenderer::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties)
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &MemProperties);
    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; ++i)
    {
        if (vTypeFilter & (1 << i) && 
            (MemProperties.memoryTypes[i].propertyFlags & vProperties))
        {
            return i;
        }
    }

    throw std::runtime_error(u8"未找到合适的存储类别");
}

void CVulkanRenderer::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout) {
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = vOldLayout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = vImage;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (__hasStencilComponent(vFormat))
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DestStage;

    if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::runtime_error(u8"不支持该布局转换");
    }

    vkCmdPipelineBarrier(
        CommandBuffer,
        SrcStage, DestStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &Barrier
    );

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

bool CVulkanRenderer::__hasStencilComponent(VkFormat vFormat) {
    return vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void CVulkanRenderer::__createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    _ASSERTE(vSize > 0);
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateBuffer(m_Device, &BufferInfo, nullptr, &voBuffer));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(m_Device, voBuffer, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voBufferMemory));

    ck(vkBindBufferMemory(m_Device, voBuffer, voBufferMemory, 0));
}

void CVulkanRenderer::__copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

void CVulkanRenderer::__copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vkCmdCopyBufferToImage(CommandBuffer, vBuffer, vImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
    
    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

size_t CVulkanRenderer::__getActualTextureNum()
{
    size_t NumTexture = m_Scene.TexImages.size();
    if (NumTexture > m_MaxTextureNum)
    {
        GlobalLogger::logStream() << u8"警告: 纹理数量 = (" << std::to_string(NumTexture) << u8") 大于限制数量 (" << std::to_string(m_MaxTextureNum) << u8"), 多出的纹理将被忽略";
        NumTexture = m_MaxTextureNum;
    }
    return NumTexture;
}

size_t CVulkanRenderer::__getActualLightmapNum()
{
    if (!m_Scene.UseLightmap) return 0;
    size_t NumLightmap = m_Scene.LightmapImages.size();
    if (NumLightmap > m_MaxLightmapNum)
    {
        GlobalLogger::logStream() << u8"警告: Lightmap数量 = (" << std::to_string(NumLightmap) << u8") 大于限制数量 (" << std::to_string(m_MaxLightmapNum) << u8"), 多出的Lightmap将被忽略";
        NumLightmap = m_MaxLightmapNum;
    }
    return NumLightmap;
}

void CVulkanRenderer::__createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    int TexWidth = vpImage->getImageWidth();
    int TexHeight = vpImage->getImageHeight();
    const void* pPixelData = vpImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;
    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pDevData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, DataSize, 0, &pDevData));
    memcpy(pDevData, pPixelData, static_cast<size_t>(DataSize));
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createImage(TexWidth, TexHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voImage, voImageMemory);
    __transitionImageLayout(voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    __copyBufferToImage(StagingBuffer, voImage, TexWidth, TexHeight);
    __transitionImageLayout(voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__calculateVisiableObjects()
{
    SFrustum Frustum = m_pCamera->getFrustum();

    if ((m_RenderMethod == ERenderMethod::BSP || m_EnableCulling) && m_EnablePVS)
        m_CameraNodeIndex = m_Scene.BspTree.getPointLeaf(m_pCamera->getPos());
    else
        m_CameraNodeIndex = std::nullopt;

    m_VisableObjectNum = 0;
    for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;
        if (m_EnableCulling)
        {
            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!__isObjectInSight(m_Scene.Objects[i], Frustum))
                    continue;

            // PVS culling
            if (m_EnablePVS)
                if (!m_Scene.BspPvs.isVisiableLeafVisiable(m_CameraNodeIndex.value(), i))
                    continue;
        }

        m_AreObjectsVisable[i] = true;
        ++m_VisableObjectNum;
    }
}

bool CVulkanRenderer::__isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const
{
    // AABB frustum culling
    const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
    S3DBoundingBox BoundingBox = vpObject->getBoundingBox();
    std::array<glm::vec3, 8> BoundPoints;
    for (int i = 0; i < 8; ++i)
    {
        float X = ((i & 1) ? BoundingBox.Min.x : BoundingBox.Max.x);
        float Y = ((i & 2) ? BoundingBox.Min.y : BoundingBox.Max.y);
        float Z = ((i & 4) ? BoundingBox.Min.z : BoundingBox.Max.z);
        BoundPoints[i] = glm::vec3(X, Y, Z);
    }

    // for each frustum plane
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 Normal = glm::vec3(FrustumPlanes[i].x, FrustumPlanes[i].y, FrustumPlanes[i].z);
        float D = FrustumPlanes[i].w;
        // if all of the vertices in bounding is behind this plane, the object should not be drawn
        bool NoDraw = true;
        for (int k = 0; k < 8; ++k)
        {
            if (glm::dot(Normal, BoundPoints[k]) + D > 0)
            {
                NoDraw = false;
                break;
            }
        }
        if (NoDraw) return false;
    }
    return true;
}

void CVulkanRenderer::recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    vkDeviceWaitIdle(m_Device);
    __destroyRecreateResources();
    m_ImageFormat = vImageFormat;
    m_Extent = vExtent;
    m_ImageViews = vImageViews;
    __createRecreateResources();
}

void CVulkanRenderer::update(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

void CVulkanRenderer::__updateUniformBuffer(uint32_t vImageIndex)
{
    static auto StartTime = std::chrono::high_resolution_clock::now();

    auto CurrentTime = std::chrono::high_resolution_clock::now();
    float DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

    float Aspect = 1.0;
    if (m_Extent.height > 0 && m_Extent.width > 0)
        Aspect = static_cast<float>(m_Extent.width) / m_Extent.height;

    m_pCamera->setAspect(Aspect);
    SUniformBufferObjectVert UBO = {};
    //UBO.Model = glm::rotate(glm::mat4(1.0), DeltaTime * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    UBO.Model = glm::mat4(1.0f);
    UBO.View = m_pCamera->getViewMat();
    UBO.Proj = m_pCamera->getProjMat();

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferMemories[vImageIndex], 0, sizeof(UBO), 0, &pData));
    memcpy(pData, &UBO, sizeof(UBO));
    vkUnmapMemory(m_Device, m_VertUniformBufferMemories[vImageIndex]);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = m_pCamera->getPos();

    ck(vkMapMemory(m_Device, m_FragUniformBufferMemories[vImageIndex], 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferMemories[vImageIndex]);
}