#include "Application.h"
#include "Log.h"
#include "AppInfo.h"

using namespace vk;

void IApplication::init(GLFWwindow* vWindow)
{
    m_pWindow = vWindow;

    __createInstance();
    if (ENABLE_VALIDATION_LAYERS) __setupDebugMessenger();
    m_pSurface->create(m_pInstance, m_pWindow);
    m_pPhysicalDevice = CPhysicalDevice::chooseBestDevice(m_pInstance, m_pSurface, m_DeviceExtensions);
    m_pDevice->create(m_pPhysicalDevice, m_DeviceExtensions, m_ValidationLayers);
    __createSemaphores();

    SPortFormat SwapchainFormat = SPortFormat::AnyFormat;
    SwapchainFormat.Usage = EUsage::UNDEFINED;
    m_pSwapchainPort = make<CSourcePort>(SwapchainFormat);
    m_pSwapchainPort->markAsSwapchainSource();
    __createSwapchain();

    _initV();

    _createOtherResourceV();
}

void IApplication::waitDevice()
{
    m_pDevice->waitUntilIdle();
}

void IApplication::destroy()
{
    if (*m_pInstance == VK_NULL_HANDLE) return;

    _destroyOtherResourceV();
    __destroySwapchain();

    for (size_t i = 0; i < m_MaxFrameInFlight; ++i)
    {
        m_pDevice->destroySemaphore(m_RenderFinishedSemaphores[i]);
        m_pDevice->destroySemaphore(m_ImageAvailableSemaphores[i]);
        m_InFlightFenceSet[i]->destroy();
    }

    m_pDevice->destroy();
    m_pSurface->destroy();
    if (ENABLE_VALIDATION_LAYERS) m_pDebugMessenger->destroy();
    m_pInstance->destroy();

    m_pWindow = nullptr;
    m_pPhysicalDevice->release();

    m_CurrentFrameIndex = 0;
    m_FramebufferResized = false;
}

void IApplication::render()
{
    m_InFlightFenceSet[m_CurrentFrameIndex]->wait();

    uint32_t ImageIndex;
    VkResult Result = vkAcquireNextImageKHR(*m_pDevice, *m_pSwapchain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &ImageIndex);

    _updateV(ImageIndex);
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

    std::vector<VkCommandBuffer> CommandBufferSet = _getCommandBufferSetV(ImageIndex);

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

    m_InFlightFenceSet[m_CurrentFrameIndex]->reset();
    vk::checkError(vkQueueSubmit(m_pDevice->getGraphicsQueue(), 1, &SubmitInfo, *m_InFlightFenceSet[m_CurrentFrameIndex]));

    VkSwapchainKHR SwapChains[] = { *m_pSwapchain };
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = SignalSemaphores;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = SwapChains;
    PresentInfo.pImageIndices = &ImageIndex;

    Result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &PresentInfo);
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
    {
        m_FramebufferResized = false;
        __recreateSwapchain();
    }
    else if (Result != VK_SUCCESS)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFrameInFlight;
}

vk::SAppInfo IApplication::getAppInfo()
{
    vk::SAppInfo Info;
    Info.pDevice = m_pDevice;
    Info.Extent = m_pSwapchain->getExtent();
    Info.ImageFormat = m_pSwapchain->getImageFormat();
    Info.ImageNum = m_pSwapchain->getImageNum();

    return Info;
}

void IApplication::_initV()
{
}

void IApplication::_updateV(uint32_t vImageIndex)
{
}

std::vector<VkCommandBuffer> IApplication::_getCommandBufferSetV(uint32_t vImageIndex)
{
    return {};
}

void IApplication::_createOtherResourceV()
{
}

void IApplication::_recreateOtherResourceV()
{
}

void IApplication::_destroyOtherResourceV()
{
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
        Common::Log::log(u8"[验证层] " + vMessage);
    };
    m_pDebugMessenger->setCustomCallback(pCallback);
} 

void IApplication::__createSwapchain()
{
    m_pSwapchain->create(m_pDevice);
    const auto& Views = m_pSwapchain->getImageViews();
    for (size_t i = 0; i < m_pSwapchain->getImageNum(); ++i)
        m_pSwapchainPort->setImage(Views[i], i);

    m_pSwapchainPort->setActualFormat(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent());
}

void IApplication::__destroySwapchain()
{
    m_pSwapchain->destroy();
}

void IApplication::__createSemaphores()
{
    m_ImageAvailableSemaphores.resize(m_MaxFrameInFlight);
    m_RenderFinishedSemaphores.resize(m_MaxFrameInFlight);
    m_InFlightFenceSet.resize(m_MaxFrameInFlight);

    for (size_t i = 0; i < m_MaxFrameInFlight; ++i)
    {
        m_ImageAvailableSemaphores[i] = m_pDevice->createSemaphore();
        m_RenderFinishedSemaphores[i] = m_pDevice->createSemaphore();
        m_InFlightFenceSet[i] = make<vk::CFence>();
        m_InFlightFenceSet[i]->create(m_pDevice, true);
    }
}

void IApplication::__recreateSwapchain()
{
    waitDevice();
    __destroySwapchain();
    __createSwapchain();
    _recreateOtherResourceV();
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

    return Extensions;
}