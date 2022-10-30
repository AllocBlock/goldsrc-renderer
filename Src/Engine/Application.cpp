#include "Application.h"

#include <queue>

#include "Log.h"
#include "AppInfo.h"
#include "RenderPass.h"

#include <set>
#include <stack>

using namespace vk;

IApplication::IApplication()
{
    m_pSwapchainPort->markAsSwapchainSource();
}

void IApplication::create(GLFWwindow* vWindow)
{
    m_pWindow = vWindow;

    __createInstance();
    if (ENABLE_VALIDATION_LAYERS) __setupDebugMessenger();
    m_pSurface->create(m_pInstance, m_pWindow);
    m_pPhysicalDevice = CPhysicalDevice::chooseBestDevice(m_pInstance, m_pSurface, m_DeviceExtensions);
    m_pDevice->create(m_pPhysicalDevice, m_DeviceExtensions, m_ValidationLayers);
    __createSemaphores();

    __createSwapchain();

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

    std::vector<VkCommandBuffer> CommandBufferSet = __sortCommandBuffers(ImageIndex);

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
    if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
    {
        __recreateSwapchain();
    }
    else if (Result != VK_SUCCESS)
    {
        throw std::runtime_error(u8"获取交换链图像失败");
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFrameInFlight;
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

void IApplication::__createSwapchain()
{
    m_pSwapchain->create(m_pDevice);
    const auto& Views = m_pSwapchain->getImageViews();

    auto ImageNum = m_pSwapchain->getImageNum();

    m_pSwapchainPort->setActualFormat(m_pSwapchain->getImageFormat());
    m_pSwapchainPort->setActualExtent(m_pSwapchain->getExtent());
    m_pSwapchainPort->setImageNum(ImageNum);
    m_pSwapchainPort->clearImage();
    for (size_t i = 0; i < ImageNum; ++i)
        m_pSwapchainPort->setImage(Views[i], i);

    m_pAppInfo->setImageNum(ImageNum);
    m_pAppInfo->setScreenExtent(m_pSwapchain->getExtent()); // for now, swapchain extent == screen extent
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
}

IRenderPass* getParentPass(CPort::Ptr vInputPort)
{
    auto pParent = vInputPort->getParent();
    if (!pParent) return nullptr;
    auto pPortSet = pParent->getBelongedPortSet();
    if (!pPortSet) return nullptr; // separate port like swapchain port
    auto pPass = pPortSet->getBelongedRenderPass();
    _ASSERTE(pPass);
    return pPass;
}

std::vector<IRenderPass*> getChildPasses(CPort::Ptr vOutputPort)
{
    std::vector<IRenderPass*> Result;
    auto ChildNum = vOutputPort->getChildNum();
    for (size_t i = 0; i < ChildNum; ++i)
    {
        auto pPort = vOutputPort->getChild(i);
        auto pPortSet = pPort->getBelongedPortSet();
        _ASSERTE(pPortSet);
        auto pPass = pPortSet->getBelongedRenderPass();
        _ASSERTE(pPass);

        Result.emplace_back(pPass);
    }
    return Result;
}

bool __isTail(IRenderPass* vPass, const std::set<IRenderPass*>& vRemovedPasses)
{
    auto pPortSet = vPass->getPortSet();
    auto OutputNum = pPortSet->getOutputPortNum();
    for (size_t i = 0; i < OutputNum; ++i)
    {
        auto pOutput = pPortSet->getOutputPort(i);
        auto Passes = getChildPasses(pOutput);
        for (auto pPass : Passes)
        {
            if (vRemovedPasses.find(pPass) == vRemovedPasses.end()) // child is not removed
                return false;
        }
    }
    return true;
}

std::set<IRenderPass*> __findAllPasses(CPort::Ptr vStartPort)
{
    std::set<IRenderPass*> AllPasses;
    std::queue<IRenderPass*> InqueuePasses;

    auto ChildPasses = getChildPasses(vStartPort);
    for (auto pPass : ChildPasses)
    {
        InqueuePasses.push(pPass);
        AllPasses.insert(pPass);
    }

    while(!InqueuePasses.empty())
    {
        auto pCurPass = InqueuePasses.front();
        InqueuePasses.pop();

        auto pPortSet = pCurPass->getPortSet();
        // process parent
        auto InputNum = pPortSet->getInputPortNum();
        for (size_t i = 0; i < InputNum; ++i)
        {
            auto pInput = pPortSet->getInputPort(i);
            auto pPass = getParentPass(pInput);
            if (pPass && AllPasses.find(pPass) == AllPasses.end())
            {
                InqueuePasses.push(pPass);
                AllPasses.insert(pPass);
            }
        }

        // process children
        auto OutputNum = pPortSet->getOutputPortNum();
        for (size_t i = 0; i < OutputNum; ++i)
        {
            auto pOutput = pPortSet->getOutputPort(i);
            auto Passes = getChildPasses(pOutput);
            for (auto pPass : Passes)
            {
                if (AllPasses.find(pPass) == AllPasses.end())
                {
                    InqueuePasses.push(pPass);
                    AllPasses.insert(pPass);
                }
            }
        }
    }

    return AllPasses;
}

std::vector<VkCommandBuffer> IApplication::__sortCommandBuffers(uint32_t vImageIndex)
{
    // 1. find all passes
    std::set<IRenderPass*> AllPasses = __findAllPasses(m_pSwapchainPort);

    // 2. loop remove tails, and find new tails
    std::set<IRenderPass*> RemovedPasses;
    std::vector<IRenderPass*> OrderedPassSet;

    while(RemovedPasses.size() != AllPasses.size())
    {
        for (auto pPass : AllPasses)
        {
            if (RemovedPasses.find(pPass) != RemovedPasses.end()) continue;

            // is tail?
            bool IsTail = __isTail(pPass, RemovedPasses);

            if (IsTail)
            {
                RemovedPasses.insert(pPass);
                OrderedPassSet.emplace_back(pPass);
            }
        }
    }

    std::vector<VkCommandBuffer> CommandBufferSet;
    for (size_t i = 0; i < OrderedPassSet.size(); ++i) // reverse
    {
        const auto& BufferSet = OrderedPassSet[i]->requestCommandBuffers(vImageIndex);
        CommandBufferSet.insert(CommandBufferSet.begin(), BufferSet.begin(), BufferSet.end());
    }

    return CommandBufferSet;
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