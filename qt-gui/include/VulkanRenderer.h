#pragma once
#include <QVulkanWindowRenderer>
#include <QVulkanDeviceFunctions>

class CVulkanRenderer : public QVulkanWindowRenderer
{
public:
    CVulkanRenderer(QVulkanWindow* vWindow);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    VkShaderModule __createShader(const QString& vName);
    void __initBuffer();
    VkPipelineVertexInputStateCreateInfo __getVertexInputInfo();
    void __initDescriptor();
    void __initPipeline();
    void __checkVkError(VkResult vErr);

    QVulkanWindow* m_pWindow = nullptr;
    QVulkanDeviceFunctions* m_pDevFuncs = nullptr;

    VkDeviceMemory m_BufferMemory = VK_NULL_HANDLE;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_UniformBufferInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_DescPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_DescSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
      
    QMatrix4x4 m_MatProj;
    float m_Rotation = 0.0f;
};

class CVulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer* createRenderer() override;
};