#include "../include/VulkanRenderer.h"

#include <QFile>

static float g_VertexData[] = { // Y up, front = CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};
static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VulkanRenderer::VulkanRenderer(QVulkanWindow* vWindow)
    : m_pWindow(vWindow)
{
}

void VulkanRenderer::initResources()
{
    qDebug("initResources");
    const VkDevice& Device = m_pWindow->device();

    m_pDevFuncs = m_pWindow->vulkanInstance()->deviceFunctions(Device);
    __initBuffer();
    VkPipelineVertexInputStateCreateInfo VertexInputInfo = __getVertexInputInfo();
    __initDescriptor();
    __initPipeline(VertexInputInfo);

}

void VulkanRenderer::__initBuffer()
{
    _ASSERTE(m_pWindow);
    qDebug("initBuffer");
    const VkDevice& Device = m_pWindow->device();
    const int ConcurrentFrameCount = m_pWindow->concurrentFrameCount();
    const VkPhysicalDeviceLimits* pDevLimits = &m_pWindow->physicalDeviceProperties()->limits;
    const VkDeviceSize UniAlign = pDevLimits->minUniformBufferOffsetAlignment;
    const VkDeviceSize VertexAllocSize = aligned(sizeof(g_VertexData), UniAlign);
    const VkDeviceSize UniformAllocSize = aligned(UNIFORM_DATA_SIZE, UniAlign); // uniform size * frame count

    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = VertexAllocSize + UniformAllocSize * ConcurrentFrameCount;
    BufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkResult Err = m_pDevFuncs->vkCreateBuffer(Device, &BufferInfo, nullptr, &m_Buffer);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", Err);

    VkMemoryRequirements MemReq;
    m_pDevFuncs->vkGetBufferMemoryRequirements(Device, m_Buffer, &MemReq);
    VkMemoryAllocateInfo MemAllocInfo = {};
    MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemAllocInfo.allocationSize = MemReq.size;
    MemAllocInfo.memoryTypeIndex = m_pWindow->hostVisibleMemoryIndex();

    Err = m_pDevFuncs->vkAllocateMemory(Device, &MemAllocInfo, nullptr, &m_BufferMemory);
    if (Err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", Err);
    Err = m_pDevFuncs->vkBindBufferMemory(Device, m_Buffer, m_BufferMemory, 0);
    if (Err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", Err);
    quint8* pDev;
    Err = m_pDevFuncs->vkMapMemory(Device, m_BufferMemory, 0, MemReq.size, 0, reinterpret_cast<void**>(&pDev));
    if (Err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", Err);
    memcpy(pDev, g_VertexData, sizeof(g_VertexData)); // copy vertex data
    QMatrix4x4 MatIdent;
    for (int i = 0; i < ConcurrentFrameCount; i++) {
        const VkDeviceSize Offset = VertexAllocSize + i * UniformAllocSize;
        memcpy(pDev + Offset, MatIdent.constData(), UNIFORM_DATA_SIZE); // copy ith uniform data
        m_UniformBufferInfo[i] = {};
        m_UniformBufferInfo[i].buffer = m_Buffer;
        m_UniformBufferInfo[i].offset = Offset;
        m_UniformBufferInfo[i].range = UniformAllocSize;
    }
    m_pDevFuncs->vkUnmapMemory(Device, m_BufferMemory);
}

VkPipelineVertexInputStateCreateInfo VulkanRenderer::__getVertexInputInfo()
{
    VkVertexInputBindingDescription VertexBindingDesc = {};
    VertexBindingDesc.binding = 0;
    VertexBindingDesc.stride = 5 * sizeof(float);
    VertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription VertexAttrDesc[2];
    VertexAttrDesc[0] = {};
    VertexAttrDesc[0].location = 0;
    VertexAttrDesc[0].binding = 0;
    VertexAttrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
    VertexAttrDesc[0].offset = 0;
    VertexAttrDesc[1] = {};
    VertexAttrDesc[1].location = 1;
    VertexAttrDesc[1].binding = 0;
    VertexAttrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    VertexAttrDesc[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
    VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.flags = 0;
    VertexInputInfo.vertexBindingDescriptionCount = 1;
    VertexInputInfo.pVertexBindingDescriptions = &VertexBindingDesc;
    VertexInputInfo.vertexAttributeDescriptionCount = 2;
    VertexInputInfo.pVertexAttributeDescriptions = VertexAttrDesc;

    return VertexInputInfo;
}

void VulkanRenderer::__initDescriptor()
{
    const VkDevice& Device = m_pWindow->device();
    const int ConcurrentFrameCount = m_pWindow->concurrentFrameCount();
    // Set up descriptor set and its layout.
    VkDescriptorPoolSize DescPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(ConcurrentFrameCount) };
    VkDescriptorPoolCreateInfo DescPoolInfo = {};
    DescPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescPoolInfo.maxSets = ConcurrentFrameCount;
    DescPoolInfo.poolSizeCount = 1;
    DescPoolInfo.pPoolSizes = &DescPoolSizes;
    VkResult Err = m_pDevFuncs->vkCreateDescriptorPool(Device, &DescPoolInfo, nullptr, &m_DescPool);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", Err);
    VkDescriptorSetLayoutBinding LayoutBinding = {};
    LayoutBinding.binding = 0;
    LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    LayoutBinding.descriptorCount = 1;
    LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo DescLayoutInfo = {};
    DescLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescLayoutInfo.flags = 0;
    DescLayoutInfo.bindingCount = 1;
    DescLayoutInfo.pBindings = &LayoutBinding;

    Err = m_pDevFuncs->vkCreateDescriptorSetLayout(Device, &DescLayoutInfo, nullptr, &m_DescSetLayout);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", Err);
    for (int i = 0; i < ConcurrentFrameCount; i++)
    {
        VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
        DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        DescSetAllocInfo.descriptorPool = m_DescPool;
        DescSetAllocInfo.descriptorSetCount = 1;
        DescSetAllocInfo.pSetLayouts = &m_DescSetLayout;

        Err = m_pDevFuncs->vkAllocateDescriptorSets(Device, &DescSetAllocInfo, &m_DescSet[i]);
        if (Err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", Err);
        VkWriteDescriptorSet DescWrite = {};
        DescWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        DescWrite.dstSet = m_DescSet[i];
        DescWrite.descriptorCount = 1;
        DescWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        DescWrite.pBufferInfo = &m_UniformBufferInfo[i];
        m_pDevFuncs->vkUpdateDescriptorSets(Device, 1, &DescWrite, 0, nullptr);
    }
}

void VulkanRenderer::__initPipeline(VkPipelineVertexInputStateCreateInfo vVertexInputInfo)
{
    const VkDevice& Device = m_pWindow->device();
    // Pipeline cache
    VkPipelineCacheCreateInfo PipelineCacheInfo = {};
    PipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult Err = m_pDevFuncs->vkCreatePipelineCache(Device, &PipelineCacheInfo, nullptr, &m_PipelineCache);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", Err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &m_DescSetLayout;
    Err = m_pDevFuncs->vkCreatePipelineLayout(Device, &PipelineLayoutInfo, nullptr, &m_PipelineLayout);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", Err);

    // Shaders
    VkShaderModule VertShaderModule = __createShader(QStringLiteral("shader/vert.spv"));
    VkShaderModule FragShaderModule = __createShader(QStringLiteral("shader/frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo ShaderStages[2];
    ShaderStages[0] = {};
    ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].flags = 0;
    ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module = VertShaderModule;
    ShaderStages[0].pName = "main";
    ShaderStages[1] = {};
    ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].flags = 0;
    ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = FragShaderModule;
    ShaderStages[1].pName = "main";
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = ShaderStages;
    PipelineInfo.pVertexInputState = &vVertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo InputAssembleCreateInfo = {};
    InputAssembleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssembleCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    PipelineInfo.pInputAssemblyState = &InputAssembleCreateInfo;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo ViewportCreateInfo = {};
    ViewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportCreateInfo.viewportCount = 1;
    ViewportCreateInfo.scissorCount = 1;
    PipelineInfo.pViewportState = &ViewportCreateInfo;

    VkPipelineRasterizationStateCreateInfo RastCreateInfo = {};
    RastCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RastCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RastCreateInfo.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    RastCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RastCreateInfo.lineWidth = 1.0f;
    PipelineInfo.pRasterizationState = &RastCreateInfo;

    VkPipelineMultisampleStateCreateInfo MultiSamplingCreateInfo = {};
    MultiSamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling.
    MultiSamplingCreateInfo.rasterizationSamples = m_pWindow->sampleCountFlagBits();
    PipelineInfo.pMultisampleState = &MultiSamplingCreateInfo;

    VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo = {};
    DepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilCreateInfo.depthTestEnable = VK_TRUE;
    DepthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    PipelineInfo.pDepthStencilState = &DepthStencilCreateInfo;

    VkPipelineColorBlendStateCreateInfo ColorBlendCreateInfo = {};
    ColorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba

    VkPipelineColorBlendAttachmentState ColorBlendAttach = {};
    ColorBlendAttach.colorWriteMask = 0xF;
    ColorBlendCreateInfo.attachmentCount = 1;
    ColorBlendCreateInfo.pAttachments = &ColorBlendAttach;
    PipelineInfo.pColorBlendState = &ColorBlendCreateInfo;
    VkDynamicState DynEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo DynamicCreateInfo = {};
    DynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicCreateInfo.dynamicStateCount = sizeof(DynEnables) / sizeof(VkDynamicState);
    DynamicCreateInfo.pDynamicStates = DynEnables;
    PipelineInfo.pDynamicState = &DynamicCreateInfo;
    PipelineInfo.layout = m_PipelineLayout;
    PipelineInfo.renderPass = m_pWindow->defaultRenderPass();
    Err = m_pDevFuncs->vkCreateGraphicsPipelines(Device, m_PipelineCache, 1, &PipelineInfo, nullptr, &m_Pipeline);
    if (Err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", Err);

    if (VertShaderModule)
        m_pDevFuncs->vkDestroyShaderModule(Device, VertShaderModule, nullptr);
    if (FragShaderModule)
        m_pDevFuncs->vkDestroyShaderModule(Device, FragShaderModule, nullptr);
} 

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    // Projection matrix
    m_MatProj = m_pWindow->clipCorrectionMatrix(); // adjust for Vulkan-OpenGL clip space differences
    const QSize Size = m_pWindow->swapChainImageSize();
    m_MatProj.perspective(45.0f, Size.width() / (float)Size.height(), 0.01f, 100.0f);
    m_MatProj.translate(0, 0, -4);
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");

    VkDevice Device = m_pWindow->device();
    if (m_Pipeline) {
        m_pDevFuncs->vkDestroyPipeline(Device, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
    if (m_PipelineLayout) {
        m_pDevFuncs->vkDestroyPipelineLayout(Device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
    if (m_PipelineCache) {
        m_pDevFuncs->vkDestroyPipelineCache(Device, m_PipelineCache, nullptr);
        m_PipelineCache = VK_NULL_HANDLE;
    }
    if (m_DescSetLayout) {
        m_pDevFuncs->vkDestroyDescriptorSetLayout(Device, m_DescSetLayout, nullptr);
        m_DescSetLayout = VK_NULL_HANDLE;
    }
    if (m_DescPool) {
        m_pDevFuncs->vkDestroyDescriptorPool(Device, m_DescPool, nullptr);
        m_DescPool = VK_NULL_HANDLE;
    }
    if (m_Buffer) {
        m_pDevFuncs->vkDestroyBuffer(Device, m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }
    if (m_BufferMemory) {
        m_pDevFuncs->vkFreeMemory(Device, m_BufferMemory, nullptr);
        m_BufferMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::startNextFrame()
{
    VkDevice Device = m_pWindow->device();
    VkCommandBuffer CommandBuffer = m_pWindow->currentCommandBuffer();
    const QSize Size = m_pWindow->swapChainImageSize();

    VkClearColorValue ClearColor = { { 0, 0, 0, 1 } };
    VkClearDepthStencilValue ClearDepthStencil = { 1, 0 };
    VkClearValue ClearValues[3];
    ClearValues[0] = {};
    ClearValues[1] = {};
    ClearValues[2] = {};
    ClearValues[0].color = ClearValues[2].color = ClearColor;
    ClearValues[1].depthStencil = ClearDepthStencil;

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_pWindow->defaultRenderPass();
    RenderPassBeginInfo.framebuffer = m_pWindow->currentFramebuffer();
    RenderPassBeginInfo.renderArea.extent.width = Size.width();
    RenderPassBeginInfo.renderArea.extent.height = Size.height();
    RenderPassBeginInfo.clearValueCount = m_pWindow->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    RenderPassBeginInfo.pClearValues = ClearValues;

    VkCommandBuffer CmdBuf = m_pWindow->currentCommandBuffer();
    m_pDevFuncs->vkCmdBeginRenderPass(CmdBuf, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    quint8* pDev;
    VkResult Err = m_pDevFuncs->vkMapMemory(Device, m_BufferMemory, m_UniformBufferInfo[m_pWindow->currentFrame()].offset,
        UNIFORM_DATA_SIZE, 0, reinterpret_cast<void**>(&pDev));
    if (Err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", Err);
    QMatrix4x4 TempMatProj = m_MatProj;
    TempMatProj.rotate(m_Rotation, 0, 1, 0);
    memcpy(pDev, TempMatProj.constData(), 16 * sizeof(float));
    m_pDevFuncs->vkUnmapMemory(Device, m_BufferMemory);
    // Not exactly a real animation system, just advance on every frame for now.
    m_Rotation += 1.0f;
    m_pDevFuncs->vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    m_pDevFuncs->vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
        &m_DescSet[m_pWindow->currentFrame()], 0, nullptr);
    VkDeviceSize VertexBufferOffset = 0;
    m_pDevFuncs->vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_Buffer, &VertexBufferOffset);
    VkViewport Viewport;
    Viewport.x = Viewport.y = 0;
    Viewport.width = Size.width();
    Viewport.height = Size.height();
    Viewport.minDepth = 0;
    Viewport.maxDepth = 1;
    m_pDevFuncs->vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
    VkRect2D Scissor;
    Scissor.offset.x = Scissor.offset.y = 0;
    Scissor.extent.width = Viewport.width;
    Scissor.extent.height = Viewport.height;
    m_pDevFuncs->vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);
    m_pDevFuncs->vkCmdDraw(CommandBuffer, 3, 1, 0, 0);
    m_pDevFuncs->vkCmdEndRenderPass(CmdBuf);
    m_pWindow->frameReady();
    m_pWindow->requestUpdate(); // render continuously, throttled by the presentation rate
}

VkShaderModule VulkanRenderer::__createShader(const QString& vName)
{
    QFile File(vName);
    if (!File.open(QIODevice::ReadOnly))
    {
        qWarning("Failed to read shader %s", qPrintable(vName));
        return VK_NULL_HANDLE;
    }
    QByteArray Blob = File.readAll();
    File.close();

    VkShaderModuleCreateInfo ShaderInfo = {};
    ShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderInfo.codeSize = Blob.size();
    ShaderInfo.pCode = reinterpret_cast<const uint32_t*>(Blob.constData());

    VkShaderModule ShaderModule;
    VkResult Err = m_pDevFuncs->vkCreateShaderModule(m_pWindow->device(), &ShaderInfo, nullptr, &ShaderModule);
    if (Err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", Err);
        return VK_NULL_HANDLE;
    }
    return ShaderModule;
}

QVulkanWindowRenderer* VulkanWindow::createRenderer()
{
    return new VulkanRenderer(this);
}