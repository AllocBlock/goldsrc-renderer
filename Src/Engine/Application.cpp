#include "Application.h"
#include "Log.h"
#include "RenderPass.h"

using namespace vk;

IApplication::IApplication()
{
}

void IApplication::create(GLFWwindow* vWindow)
{
    m_pWindow = vWindow;

    __createInstance();
    if (ENABLE_VALIDATION_LAYERS) __setupDebugMessenger();
    m_pSurface->create(m_pInstance, m_pWindow);
    m_pPhysicalDevice = CPhysicalDevice::chooseBestDevice(m_pInstance, m_pSurface, m_DeviceExtensions);
    m_pDevice->create(m_pPhysicalDevice, m_DeviceExtensions, m_ValidationLayers);

    m_pSwapchain->create(m_pDevice);
    __createSemaphores();

    m_pGraphInstance->init(m_pDevice, m_pSwapchain->getExtent(), m_pSceneInfo);

    _createV();
}

void IApplication::waitDevice()
{
    m_pDevice->waitUntilIdle();
}

void IApplication::destroy()
{
    if (*m_pInstance == VK_NULL_HANDLE) return;
    _destroyV();

    m_pGraphInstance->destroy();
    m_pSwapchain->destroy();

    destroyAndClear(m_pInFlightFence);
    for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
    {
        m_pDevice->destroySemaphore(m_RenderFinishedSemaphores[i]);
        m_pDevice->destroySemaphore(m_ImageAvailableSemaphores[i]);
    }

    m_pDevice->destroy();
    m_pSurface->destroy();
    if (ENABLE_VALIDATION_LAYERS) m_pDebugMessenger->destroy();
    m_pInstance->destroy();

    m_pWindow = nullptr;
    m_pPhysicalDevice->release();

    m_CurrentFrameIndex = 0;
}


void IApplication::tick()
{
    if (!m_Freezed)
    {
        __render();
    }
}

void IApplication::resize(uint32_t vWidth, uint32_t vHeight)
{
    // TIPS: no need to recreate swapchain when minimization, swapchain is still valid
    if (vWidth > 0 && vHeight > 0)
    {
        m_Freezed = false;
    }
    else
        m_Freezed = true;
}

void IApplication::__render()
{
    m_pInFlightFence->wait();

    uint32_t ImageIndex;
    VkResult Result = vkAcquireNextImageKHR(*m_pDevice, *m_pSwapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &ImageIndex);

    m_pGraphInstance->update();
    _updateV();
    _renderUIV();

    if (Result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        __recreateSwapchain();
        return;
    }
    else if (Result != VK_SUCCESS && Result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }

    m_pGraphInstance->updateSwapchainImageIndex(ImageIndex);
    std::vector<VkCommandBuffer> CommandBufferSet;
    for (auto pPass : m_pGraphInstance->getSortedPasses())
    {
        const auto& BufferSet = pPass->requestCommandBuffers();
        CommandBufferSet.insert(CommandBufferSet.end(), BufferSet.begin(), BufferSet.end());
    }

    VkSemaphore WaitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIndex] };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask = WaitStages;
    SubmitInfo.commandBufferCount = static_cast<uint32_t>(CommandBufferSet.size());
    SubmitInfo.pCommandBuffers = CommandBufferSet.data();

    VkSemaphore SignalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrameIndex] };
    SubmitInfo.signalSemaphoreCount = 1;
    SubmitInfo.pSignalSemaphores = SignalSemaphores;

    m_pInFlightFence->reset();
    vk::checkError(vkQueueSubmit(m_pDevice->getGraphicsQueue(), 1, &SubmitInfo, *m_pInFlightFence));

    VkSwapchainKHR SwapChains[] = { *m_pSwapchain };
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    PresentInfo.pImageIndices = &ImageIndex;

    Result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &PresentInfo);
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
    {
        __recreateSwapchain();
    }
    else if (Result != VK_SUCCESS)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_pSwapchain->getImageNum();
}

void IApplication::__createInstance()
{
    std::vector<const char*> Extensions = __getRequiredExtensions();

    m_pInstance = make<vk::CInstance>();

    std::vector<const char*> ValidationLayers;
    if (ENABLE_VALIDATION_LAYERS)
        ValidationLayers = m_ValidationLayers;
    m_pInstance->create("Base Instance", ValidationLayers, Extensions);
}

void IApplication::__setupDebugMessenger()
{
    m_pDebugMessenger = make<vk::CDebugMessenger>();
    m_pDebugMessenger->create(*m_pInstance);
    vk::DebugMessageCallbackFunc_t pCallback = [=](vk::EDebugMessageServerity vServerity, std::string vMessage)
    {
        Log::log("[验证层] " + vMessage);
    };
    m_pDebugMessenger->setCustomCallback(pCallback);
} 

void IApplication::__createSemaphores()
{
    _ASSERT(m_pSwapchain);
    m_pInFlightFence = make<vk::CFence>();
    m_pInFlightFence->create(m_pDevice, true);

    int ImageNum = m_pSwapchain->getImageNum();
    m_ImageAvailableSemaphores.resize(ImageNum);
    m_RenderFinishedSemaphores.resize(ImageNum);

    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_ImageAvailableSemaphores[i] = m_pDevice->createSemaphore();
        m_RenderFinishedSemaphores[i] = m_pDevice->createSemaphore();
    }
}

void IApplication::__recreateSwapchain()
{
    waitDevice();
    m_pSwapchain->destroy();
    m_pSwapchain->create(m_pDevice);
    _onSwapchainRecreateV();
}

std::vector<const char*> IApplication::__getRequiredExtensions()
{
    // 获取GLFW所需的扩展
    uint32_t GlfwExtensionCount = 0;
    const char** GlfwExtensions;
    GlfwExtensions = glfwGetRequiredInstanceExtensions(&GlfwExtensionCount);

    std::vector<const char*> Extensions(GlfwExtensions, GlfwExtensions + GlfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS) {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    Extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return Extensions;
}