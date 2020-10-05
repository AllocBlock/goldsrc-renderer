#include "VulkanApp.h"

#include <iostream>
using std::cout;
using std::endl;
using std::cerr;
using std::numeric_limits;

inline void outputTips(string text)
{
	// 本函数用于方便与格式化输出
	cout << "- " << text << endl;
}

/*******************************
* function: constructor
*/
CVulkanApp::CVulkanApp() {
	m_Instance = {};
	m_PhysicalDevices = {};
	m_DebugMessenger = {};
	m_PhysicalDevice = {};
	m_Device = {};
	m_GraphicsQueue = {};
	m_PresentQueue = {};
	m_Surface = {};
	m_SwapChain = {};
	m_SwapChainImages = {};
	m_SwapChainExtent = {};
	m_SwapChainImageViews = {};
	m_RenderPass = {};
	m_PipelineLayout = {};
	m_GraphicsPipeline = {};
	m_SwapChainFramebuffers = {};
	m_CommandPool = {};
	m_CommandBuffers = {};
	m_ImageAvailableSemaphores = {};
	m_RenderFinishedSemaphores = {};
	m_InFlightFences = {};
	m_VertexBuffer = {};
	m_VertexBufferMemory = {};
	m_IndexBuffer = {};
	m_IndexBufferMemory = {};
}

/*******************************
* required: vExtensions
* assigned: all vulkan instance member variable
*/
void				CVulkanApp::initVulkan(vector<const char*> vExtensions) {
	// vulkan的初始化，相当于main函数
	outputTips("------------------------------Vulkan初始化------------------------------");

	//初始化Vulkan
	_ASSERT(__createInstance(vExtensions) == VK_SUCCESS); // 创建实例
#if _DEBUG
	_ASSERT(__setupDebugMessenger() == VK_SUCCESS); // 设置debug messenger
#endif
	_ASSERT(__createSurface() == VK_SUCCESS); // 创建surface
	_ASSERT(__choosePhysicalDevice() == VK_SUCCESS); // 获取物理设备

	_ASSERT(__createDevice() == VK_SUCCESS); // 创建逻辑设备

	_ASSERT(__createSwapChain() == VK_SUCCESS); // 创建swap chain
	_ASSERT(__createImageViews() == VK_SUCCESS); // 创建image view
	_ASSERT(__createRenderPass() == VK_SUCCESS); // 创建Render pass

	//_ASSERT(__createDescriptorSetLayout() == VK_SUCCESS); // 创建Descriptor Set Layout
	_ASSERT(__createGraphicsPipeline() == VK_SUCCESS); // 创建管线

	_ASSERT(__createFramebuffers() == VK_SUCCESS); // 创建帧缓存
	_ASSERT(__createCommandPool() == VK_SUCCESS); // 创建命令池
	__createVertexBuffer(); // 创建顶点缓存
	__createIndexBuffer(); // 创建索引缓存
	//_ASSERT(__createDescriptorPool() == VK_SUCCESS); // 创建描述符池
	//_ASSERT(__createDescriptorSets() == VK_SUCCESS); // 创建描述符集

	_ASSERT(__createCommandBuffers() == VK_SUCCESS); // 创建命令缓存
	_ASSERT(__createSemaphores() == VK_SUCCESS); // 创建信号量
}

/*******************************
* assigned: m_pWindow
*/
void					CVulkanApp::initWindow() {
	outputTips("------------------------------窗口初始化------------------------------");

	// 初始化显示窗口
	glfwInit();																						// 初始化GLFW库

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);													// GLFW默认创建OpenGL，这里是告诉它不要创建OpenGL
	m_pWindow = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);					// 创建窗口

	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, framebufferResizeCallback);
	return;
}

/*******************************
* 
*/
void					CVulkanApp::drawFrame() {
	// drawFrame会完成：从交换链获取图像、在图像上执行命令缓存的命令、返回图像给交换链以准备显示

	// 这些事情实际是异步执行的，因此先后顺序是未知的，但我们的程序是同步的，下一步依赖上一步的结果，这边导致了同步问题。
	// 有两种方法开同步这些事件：fences and semaphores ，他们都能让一个事件置于signaled状态，而其他事件处于unsignaled状态
	// 区别在于，fences能用vkWaitForFences这样的函数被我们使用，而semaphores不能

	vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, numeric_limits<uint64_t>::max());
	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		__recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// presentation
	VkSwapchainKHR swapChains[] = { m_SwapChain };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
		m_FramebufferResized = false;
		__recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*******************************
* required: m_pWindow, m_Device
*/
void					CVulkanApp::mainLoop() {
	outputTips("-------------------------------绘制主循环-------------------------------");
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();							// 处理挂起事件有轮询和等待两种，因为游戏每帧一渲染，使用轮询更好
		drawFrame();
	}
	vkDeviceWaitIdle(m_Device); // 等到所有同步函数运行完成后再关闭
}

/*******************************
* required:
* assigned:
*/
void					CVulkanApp::cleanup() {
	__cleanupSwapChain();

	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
	vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	vkDestroyDevice(m_Device, nullptr);					// 销毁逻辑设备
	if (_DEBUG) {
		__destroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);	// 销毁surface，GLFW没有销毁surface的函数，但vulkan有
	vkDestroyInstance(m_Instance, nullptr);				// 销毁vulkan实例
	glfwDestroyWindow(m_pWindow);							// 销毁GLFW窗口
	glfwTerminate();									// 关闭GLFW
}

/*******************************
* required:
* assigned:
*/
void					CVulkanApp::framebufferResizeCallback(GLFWwindow* vWindow, int vWidth, int vHeight) {
	auto app = reinterpret_cast<CVulkanApp*>(glfwGetWindowUserPointer(vWindow));
	app->m_FramebufferResized = true;
}

/*******************************
*
*/
const int g_AllowedServerity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
VKAPI_ATTR VkBool32 VKAPI_CALL CVulkanApp::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData)
{
	string Severity;
	switch (vMessageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		Severity = "Verbose"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		Severity = "Info"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		Severity = "Warning"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		Severity = "Error"; break;
	default:
		Severity = "Unknown"; break;
	}

	if ((g_AllowedServerity & vMessageSeverity) != 0)
		cerr << "|Validation Layer| [" << Severity << "] " << vpCallbackData->pMessage << "|" << endl;

	return VK_FALSE;
}


/*******************************
* required: vExtensions(glfw)
* assigned: m_Instance
*/
VkResult				CVulkanApp::__createInstance(vector<const char*> vExtensions) {
	// 实例（instance）是建立应用与vulkan库之间联系的东西
	outputTips("创建实例...");
	// 大部分vulkan对象的创建都遵循这样的规则，新建一个对应对象，然后使用一个info对象来描述其属性，最后将其传入相应的函数
	VkApplicationInfo InfoApp = { };														// 应用程序信息，用在instance创建上
	InfoApp.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;		// sType是大部分info结构都有的属性，用于区分不同info的类型
	InfoApp.pApplicationName = "Hello Triangle";							// 应用程序名称，标记用
	InfoApp.applicationVersion = 1;										// 应用程序版本，标记用
	InfoApp.apiVersion = VK_MAKE_VERSION(1, 0, 0);					// 应用程序理论所需的最低vulkan版本，这里直接用当前版本代替

	// 创建实例
	VkInstanceCreateInfo CreateInfoInstance = { };										// 实例创建信息
	CreateInfoInstance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// 同appinfo
	CreateInfoInstance.pApplicationInfo = &InfoApp;									// 应用程序信息

#if _DEBUG
	_ASSERT(__isValidationLayerSupport());
	// 启用layers
	CreateInfoInstance.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
	CreateInfoInstance.ppEnabledLayerNames = m_ValidationLayers.data();
#else
	CreateInfoInstance.enabledLayerCount = 0;
#endif

#if _DEBUG
	vExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	CreateInfoInstance.enabledExtensionCount = static_cast<uint32_t>(vExtensions.size());
	CreateInfoInstance.ppEnabledExtensionNames = vExtensions.data();

	return vkCreateInstance(&CreateInfoInstance, nullptr, &m_Instance);						// 创建实例
}

/*******************************
* 
*/
VkResult				CVulkanApp::__createDebugUtilsMessengerEXT(VkInstance vInstance, const VkDebugUtilsMessengerCreateInfoEXT* vpCreateInfo, const VkAllocationCallbacks* vpAllocator, VkDebugUtilsMessengerEXT* vpDebugMessenger) {
	auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT");
	// vkGetInstanceProcAddr返回对应函数名的函数指针，返回的类型为PFN_vkVoidFunction，需要自己转换
	if (Func != nullptr)
		return Func(vInstance, vpCreateInfo, vpAllocator, vpDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

/*******************************
* 
*/
void					CVulkanApp::__destroyDebugUtilsMessengerEXT(VkInstance m_Instance, VkDebugUtilsMessengerEXT m_DebugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (Func != nullptr)
		Func(m_Instance, m_DebugMessenger, pAllocator);
}

/*******************************
* required: m_Instance
* assigned: m_DebugMessenger
*/
VkResult					CVulkanApp::__setupDebugMessenger() {
	outputTips("创建调试器...");
	VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	// messageSeverity表示会触发回调函数的消息等级
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 有关Vulkan loader、layers和drivers的所有诊断信息都会被获取
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT 类似资源详情之类的报告式的消息会被获取
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT warning信息，可能导致bug，也可能只需要忽略
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT error信息
	CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	// messageType表示会触发回调函数的种类
	// VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT specifies that some general event has occurred. This is typically a non-specification, non-performance event.
	// VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT specifies that something has occurred during validation against the Vulkan specification that may indicate invalid behavior.
	// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT specifies a potentially non-optimal use of Vulkan, e.g. using vkCmdClearColorImage when setting VkAttachmentDescription::loadOp to VK_ATTACHMENT_LOAD_OP_CLEAR would have worked.
	CreateInfo.pfnUserCallback = debugCallback;				// 对应的回调函数名
	CreateInfo.pUserData = nullptr;						// 对应回调函数传入的参数，默认为空

	return __createDebugUtilsMessengerEXT(m_Instance, &CreateInfo, nullptr, &m_DebugMessenger);
}

/*******************************
* required: m_Instance, m_pWindow
* assigned: m_Surface
*/
VkResult				CVulkanApp::__createSurface() {
	// surface可以理解为界面，即一个展示图像的区域
	outputTips("创建Surface...");

	// 创建surface
	return glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface);
}

/*******************************
* required: m_Instance
* assigned: m_PhysicalDevices, m_PhysicalDevice
*/
VkResult				CVulkanApp::__choosePhysicalDevice() {
	// 获取物理设备
	outputTips("获取Pysical Device...");

	// 物理设备即真实存在的如显卡、CPU的硬件
	uint32_t PhysicalDeviceCount = 0;
	VkResult Result = vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, nullptr);	// 获取物理设备（Physical Device）数量
	if (Result == VK_SUCCESS) {
		m_PhysicalDevices.resize(PhysicalDeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, m_PhysicalDevices.data()); // 获取物理设备（Physical Device）
	}
	else return Result;

	// 选择合适的物理设备
	for (const auto& device : m_PhysicalDevices) {
		if (__isDeviceSuitable(device)) {
			m_PhysicalDevice = device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	return VK_SUCCESS;
}

/*******************************
* required: m_PhysicalDevice
* assigned: m_Device, m_GraphicsQueue, m_PresentQueue
*/
VkResult				CVulkanApp::__createDevice() {
	outputTips("创建逻辑设备...");

	VkPhysicalDeviceProperties PhysicalDeviceProp; // 物理设备属性
	VkPhysicalDeviceFeatures PhysicalDeviceFeature; // 物理设备特性
	VkPhysicalDeviceMemoryProperties  PhysicalDeviceMemProp; // 物理设备内存属性
	__getPhysicalDeviceInfo(PhysicalDeviceProp, PhysicalDeviceFeature, PhysicalDeviceMemProp); // 获取物理设备信息

	SQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);

	vector<VkDeviceQueueCreateInfo> CreateInfosQueue;
	set<uint32_t> UniqueQueueFamilies = { Indices.GraphicsFamily.value(), Indices.PresentFamily.value() };

	float QueuePriority = 1.0f;
	for (uint32_t queueFamily : UniqueQueueFamilies) {
		VkDeviceQueueCreateInfo CreateInfoQueue = {};
		CreateInfoQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		CreateInfoQueue.queueFamilyIndex = queueFamily;
		CreateInfoQueue.queueCount = 1;
		CreateInfoQueue.pQueuePriorities = &QueuePriority; // 命令优先级，即使只有一个queue也需要填写
		CreateInfosQueue.push_back(CreateInfoQueue);
	}

	// 创建逻辑设备
	VkPhysicalDeviceFeatures RequiredFeatures = {}; // 所需的特性

	RequiredFeatures.multiDrawIndirect = PhysicalDeviceFeature.multiDrawIndirect;
	RequiredFeatures.tessellationShader = VK_TRUE;
	RequiredFeatures.geometryShader = VK_TRUE;
	RequiredFeatures.samplerAnisotropy = VK_TRUE; // 各向异性滤波器

	VkDeviceCreateInfo CreateInfoDevice = {};
	CreateInfoDevice.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfoDevice.queueCreateInfoCount = static_cast<uint32_t>(CreateInfosQueue.size());
	CreateInfoDevice.pQueueCreateInfos = CreateInfosQueue.data();
	CreateInfoDevice.pEnabledFeatures = &RequiredFeatures;
	// 注意：新版的vulkan已经没有instance与device的layer、extension的区别了，device的这些属性会被忽略，但为了向下兼容还是可以设置
	CreateInfoDevice.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
	CreateInfoDevice.ppEnabledExtensionNames = m_DeviceExtensions.data();

	if (_DEBUG) {
		CreateInfoDevice.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		CreateInfoDevice.ppEnabledLayerNames = m_ValidationLayers.data();
	}
	else {
		CreateInfoDevice.enabledLayerCount = 0;
	}

	VkResult Result = vkCreateDevice(m_PhysicalDevice, &CreateInfoDevice, nullptr, &m_Device);
	if (Result != VK_SUCCESS) return Result;

	vkGetDeviceQueue(m_Device, Indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, Indices.PresentFamily.value(), 0, &m_PresentQueue);

	return VK_SUCCESS;
}

/*******************************
* required: m_PhysicalDevice, m_Surface, m_Device
* assigned: m_SwapChain, m_SwapChainImages, m_SwapChainImageFormat, m_SwapChainExtent
*/
VkResult				CVulkanApp::__createSwapChain() {
	// swap chain本质是帧缓存，一般是显卡内存，用于稳定帧数等用途
	// swap chain至少有两个buffer，一个为screenbuffer，负责渲染并输出，另一个是backbuffers。每次显示一帧时，backbuffer中的内容会替换screenbuffer，这个过程叫做presentation或者swaping
	outputTips("创建Swap Chain...");

	// 使用swap chain之前需要查询其信息
	SSwapChainSupportDetails SwapChainSupport
	= __querySwapChainSupport(m_PhysicalDevice);

	uint32_t PresentModesCount = static_cast<uint32_t>(SwapChainSupport.PresentModes.size());
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &PresentModesCount, SwapChainSupport.PresentModes.data());
	// VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.
	// VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank".
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
	// VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to implement triple buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync that uses double buffering.

	// 创建swap chain
	VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.Formats); // 选择surface格式
	VkPresentModeKHR PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes); // 选择显示模式
	VkExtent2D Extent = __chooseSwapExtent(SwapChainSupport.Capabilities); // 决定swapchain区域大小


	// imageCount决定有多少images在swap chain里，这里使用所需的最小值
	// 但如果只用最小值意味着sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. 所以+1

	uint32_t ImageCount = SwapChainSupport.Capabilities.minImageCount + 1;

	// 还需要确保不会超出max（max>=min），max为0意味着无上限，需排除在外
	uint32_t MaxImageCount = SwapChainSupport.Capabilities.maxImageCount;
	if (MaxImageCount > 0 && ImageCount > MaxImageCount) {
		ImageCount = MaxImageCount;
	}

	VkSwapchainCreateInfoKHR CreateInfoSwapChain = {};
	CreateInfoSwapChain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	CreateInfoSwapChain.surface = m_Surface;
	CreateInfoSwapChain.minImageCount = ImageCount;
	CreateInfoSwapChain.imageFormat = SurfaceFormat.format;
	CreateInfoSwapChain.imageColorSpace = SurfaceFormat.colorSpace;
	CreateInfoSwapChain.imageExtent = Extent;
	CreateInfoSwapChain.imageArrayLayers = 1;

	// imageUsage specifies what kind of operations we'll use the images in the swap chain for
	// VK_IMAGE_USAGE_TRANSFER_SRC_BIT specifies that the image can be used as the source of a transfer command.
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT specifies that the image can be used as the destination of a transfer command.
	// VK_IMAGE_USAGE_SAMPLED_BIT specifies that the image can be used to create a VkImageView suitable for occupying a VkDescriptorSet slot either of type VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, and be sampled by a shader.
	// VK_IMAGE_USAGE_STORAGE_BIT specifies that the image can be used to create a VkImageView suitable for occupying a VkDescriptorSet slot of type VK_DESCRIPTOR_TYPE_STORAGE_IMAGE.
	// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for use as a color or resolve attachment in a VkFramebuffer.
	// VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for use as a depth/stencil or depth/stencil resolve attachment in a VkFramebuffer.
	// VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT specifies that the memory bound to this image will have been allocated with the VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT (see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#memory for more detail). This bit can be set for any image that can be used to create a VkImageView suitable for use as a color, resolve, depth/stencil, or input attachment.
	// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for occupying VkDescriptorSet slot of type VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; be read from a shader as an input attachment; and be used as an input attachment in a framebuffer.
	// VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV specifies that the image can be used to create a VkImageView suitable for use as a shading rate image.
	CreateInfoSwapChain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	SQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);
	uint32_t QueueFamilyIndices[] = { Indices.GraphicsFamily.value(), Indices.PresentFamily.value() };

	if (Indices.GraphicsFamily != Indices.PresentFamily) {
		CreateInfoSwapChain.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		CreateInfoSwapChain.queueFamilyIndexCount = 2;
		CreateInfoSwapChain.pQueueFamilyIndices = QueueFamilyIndices;
	}
	else {
		CreateInfoSwapChain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		CreateInfoSwapChain.queueFamilyIndexCount = 0; // Optional
		CreateInfoSwapChain.pQueueFamilyIndices = nullptr; // Optional
	}
	CreateInfoSwapChain.preTransform = SwapChainSupport.Capabilities.currentTransform;	// 基本变换如旋转、镜像
	CreateInfoSwapChain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// alpha通道模式，本模式弃用alpha，即默认alpha都为1.0
	CreateInfoSwapChain.presentMode = PresentMode;
	CreateInfoSwapChain.clipped = VK_TRUE;											// 是否弃用对不可见图像部分的命令操作
	CreateInfoSwapChain.oldSwapchain = VK_NULL_HANDLE;									// ？ 可帮助资源复用

	VkResult Result = vkCreateSwapchainKHR(m_Device, &CreateInfoSwapChain, nullptr, &m_SwapChain);
	if (Result != VK_SUCCESS) return Result;

	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, nullptr);
	m_SwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, m_SwapChainImages.data()); // 获取到swap chain的image并存储

	m_SwapChainImageFormat = SurfaceFormat.format; // 保存图像格式
	m_SwapChainExtent = Extent; // 保存区域信息
	
	return VK_SUCCESS;
}

/*******************************
* required: m_SwapChainImages
* assigned: m_SwapChainImageViews, m_SwapChainImageFormat, m_SwapChainExtent
*/
VkResult				CVulkanApp::__createImageViews() {
	outputTips("创建Image View...");

	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	// 为每一个image创建对应的imageview
	VkResult Result;
	for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
		Result = __createImageView(m_SwapChainImageViews[i], m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		if (Result != VK_SUCCESS) return Result;
	}

	return VK_SUCCESS;
}

/*******************************
* required: m_Device
* assigned: voShaderModule
*/
VkResult			CVulkanApp::__createShaderModule(VkShaderModule& voShaderModule, const vector<char>& vCode) {
	// 创建着色器模块
	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = vCode.size(); // 着色器代码大小
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(vCode.data()); // 着色器代码，reinterpret_cast强制类型转换

	return vkCreateShaderModule(m_Device, &CreateInfo, nullptr, &voShaderModule);
}

/*******************************
* required: m_Device, m_SwapChainImageFormat
* assigned: m_RenderPass
*/
VkResult				CVulkanApp::__createRenderPass() {
	// render pass告诉vulkan有哪些帧缓存附件（framebuffers attachment）会被用于渲染

	outputTips("创建Render pass...");
	VkAttachmentDescription ColorAttachment = {};
	ColorAttachment.format = m_SwapChainImageFormat;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// loadOp和storeOp决定在渲染前和渲染后对附件的操作
	// The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.
	ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
	ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Subpasses and attachment references, A single render pass can consist of multiple subpasses. 
	// Subpasses are subsequent rendering operations that depend on the contents of framebuffers in 
	// previous passes, for example a sequence of post-processing effects that are applied one after another. 
	// If you group these rendering operations into one render pass, then Vulkan is able to reorder the operations 
	// and conserve memory bandwidth for possibly better performance. For our very first triangle, however, 
	// we'll stick to a single subpass.
	VkAttachmentReference ColorAttachmentRef = {};
	ColorAttachmentRef.attachment = 0;											// The attachment parameter specifies which attachment to reference by its index in the attachment descriptions array.
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachmentRef;

	VkSubpassDependency Dependency = {};
	Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	Dependency.dstSubpass = 0;
	Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.srcAccessMask = 0;
	Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	array<VkAttachmentDescription, 1> Attachments = { ColorAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
	renderPassInfo.pAttachments = Attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &Subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &Dependency;

	return vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
}

/*******************************
* required: m_Device, shader, SVertex, m_DescriptorSetLayout, m_RenderPass, m_SwapChainExtent
* assigned: m_PipelineLayout, m_GraphicsPipeline
*/
VkResult				CVulkanApp::__createGraphicsPipeline() {
	// 图形管线（或渲染管线）是由模型到显示器之间的处理流程，涉及软硬件的处理
	// 纯数据->顶点处理（包括顶点着色器）、光栅化（顶点连接，3D转2D，处理的东西转成像素）、片元处理（包括片元着色器）、输出到帧缓存，这些操作可能是并行的
	outputTips("创建图像管线...");

	// 读取着色器，vulkan着色器要求SPIR-V格式，我们使用GLSL可读语言编写并使用glslangValidator.exe将其转为SPIR-V格式
	auto VertShaderCode = readFile("shader/vert.spv");
	auto FragShaderCode = readFile("shader/frag.spv");

	// 要将着色器传输到管线里，需要将其放在VkShaderModule对象里
	VkShaderModule VertShaderModule, FragShaderModule;
	VkResult Result;
	Result = __createShaderModule(VertShaderModule, VertShaderCode);
	if (Result != VK_SUCCESS) return Result;
	Result = __createShaderModule(FragShaderModule, FragShaderCode);
	if (Result != VK_SUCCESS) return Result;

	// 要使用着色器，需要将其分配到管线上才行
	VkPipelineShaderStageCreateInfo CreateInfoVertShaderStage = {};
	CreateInfoVertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	CreateInfoVertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	CreateInfoVertShaderStage.module = VertShaderModule;
	CreateInfoVertShaderStage.pName = "main"; // 主函数名称，一般为main

	VkPipelineShaderStageCreateInfo CreateInfoFragShaderStage = {};
	CreateInfoFragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	CreateInfoFragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	CreateInfoFragShaderStage.module = FragShaderModule;
	CreateInfoFragShaderStage.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { CreateInfoVertShaderStage, CreateInfoFragShaderStage };


	auto BindingDescription = SVertex::getBindingDescription();
	auto AttributeDescriptions = SVertex::getAttributeDescriptions();

	// VkPipelineVertexInputStateCreateInfo描述了传入着色器的数据的格式
	VkPipelineVertexInputStateCreateInfo CreateInfoVertexInput = {};
	CreateInfoVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	CreateInfoVertexInput.vertexBindingDescriptionCount = 1;
	CreateInfoVertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
	CreateInfoVertexInput.pVertexBindingDescriptions = &BindingDescription;
	CreateInfoVertexInput.pVertexAttributeDescriptions = AttributeDescriptions.data();

	// Input assembly图元装配 ,The VkPipelineInputAssemblyStateCreateInfo 描述了两件事：绘制模式（线、三角面等）以及图元重启

	// primitive restart图元重启： https://segmentfault.com/a/1190000012271175

	VkPipelineInputAssemblyStateCreateInfo CreateInfoInputAssembly = {};
	CreateInfoInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	CreateInfoInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;				// 每三个点为一个三角面
	CreateInfoInputAssembly.primitiveRestartEnable = VK_FALSE;							// 图元重启关闭

	// Viewports视区（可视区域） and scissors（裁剪）：视区描述了帧缓存中会被渲染的区域，一般就是整个帧缓存(0, 0)到(width, height)
	// 控制可视区域相当于对1x1方框的平移缩放，而裁剪是在其基础上的删除
	// 尝试控制宽高比
	VkViewport Viewport = {};
	float Ratio = (float)m_SwapChainExtent.width / (float)m_SwapChainExtent.height;
	float CurrentWidth = (float)m_SwapChainExtent.width;
	float CurrentHeight = (float)m_SwapChainExtent.height;

	// 自动调整比例
	if (CurrentWidth / Ratio > CurrentHeight) {
		Viewport.x = 0.0f;
		Viewport.y = (CurrentHeight - CurrentWidth / Ratio) / 2;
		Viewport.width = CurrentWidth;
		Viewport.height = CurrentWidth / Ratio;
	}
	else {
		Viewport.x = (CurrentWidth - CurrentHeight * Ratio) / 2;
		Viewport.y = 0.0f;
		Viewport.width = CurrentHeight * Ratio;
		Viewport.height = CurrentHeight;
	}

	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	// scissor裁剪确定了哪个区域内的像素会被保留
	VkRect2D Scissor = {};
	Scissor.offset = { 0, 0 };
	Scissor.extent = m_SwapChainExtent;

	// 视区和裁剪需要整合到一个viewport state里
	VkPipelineViewportStateCreateInfo CreateInfoViewportState = {};
	CreateInfoViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	CreateInfoViewportState.viewportCount = 1;
	CreateInfoViewportState.pViewports = &Viewport;
	CreateInfoViewportState.scissorCount = 1;
	CreateInfoViewportState.pScissors = &Scissor;

	// 光栅化将3D转2D像素，同时进行了深度测试、隐藏面消除等，然后把数据送入片元着色器染色
	VkPipelineRasterizationStateCreateInfo CreateInfoRasterizer = {};
	CreateInfoRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// depthClamp深度截取，取消z轴0-1的限制，在近平面之前、远平面之后的都会显示  https://blog.csdn.net/yangyong0717/article/details/78321968
	CreateInfoRasterizer.depthClampEnable = VK_FALSE;
	// 光栅化前丢弃所有图元(相当于输出空白？）
	CreateInfoRasterizer.rasterizerDiscardEnable = VK_FALSE;
	// polygonMode绘制模式
	// VK_POLYGON_MODE_FILL 填充,一定支持
	// VK_POLYGON_MODE_LINE 边被绘制为实现,需要显卡支持
	// VK_POLYGON_MODE_POINT 顶点绘制为圆点,需要显卡支持
	CreateInfoRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	CreateInfoRasterizer.lineWidth = 1.0f;
	CreateInfoRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;				// 隐藏面消除
	//CreateInfoRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//CreateInfoRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;		// 正面的指定方式,右手定则
	CreateInfoRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// 用于shadow mapping,现在我们不用
	CreateInfoRasterizer.depthBiasEnable = VK_FALSE;
	CreateInfoRasterizer.depthBiasConstantFactor = 0.0f; // Optional
	CreateInfoRasterizer.depthBiasClamp = 0.0f; // Optional
	CreateInfoRasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// MSAA抗锯齿
	VkPipelineMultisampleStateCreateInfo CreateInfoMultisample = {};
	CreateInfoMultisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	CreateInfoMultisample.sampleShadingEnable = VK_FALSE;
	CreateInfoMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	CreateInfoMultisample.minSampleShading = 1.0f; // Optional
	CreateInfoMultisample.pSampleMask = nullptr; // Optional
	CreateInfoMultisample.alphaToCoverageEnable = VK_FALSE; // Optional
	CreateInfoMultisample.alphaToOneEnable = VK_FALSE; // Optional

	// 深度测试和模板测试
	// 深度测试确保了前后顺序
	// 模板测试,在模板缓存内写入不同的值,在模板测试时会根据值得不同决定弃用还是保留像素
	VkPipelineDepthStencilStateCreateInfo CreateInfoDepthStencil = {};
	CreateInfoDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	CreateInfoDepthStencil.depthTestEnable = VK_TRUE;
	CreateInfoDepthStencil.depthWriteEnable = VK_TRUE;
	CreateInfoDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	CreateInfoDepthStencil.depthBoundsTestEnable = VK_FALSE;
	CreateInfoDepthStencil.stencilTestEnable = VK_FALSE;


	// 在片元着色器返回颜色后,需要与帧缓存现有的颜色混合
	VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
	ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	ColorBlendAttachment.blendEnable = VK_FALSE;
	ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	/*if (blendEnable) {
		finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
		finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
	} else {
		finalColor = newColor;
	}

	finalColor = finalColor & colorWriteMask;*/

	// 第二种方式,这里不用
	VkPipelineColorBlendStateCreateInfo CreateInfoColorBlending = {};
	CreateInfoColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	CreateInfoColorBlending.logicOpEnable = VK_FALSE;
	CreateInfoColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	CreateInfoColorBlending.attachmentCount = 1;
	CreateInfoColorBlending.pAttachments = &ColorBlendAttachment;
	CreateInfoColorBlending.blendConstants[0] = 0.0f; // Optional
	CreateInfoColorBlending.blendConstants[1] = 0.0f; // Optional
	CreateInfoColorBlending.blendConstants[2] = 0.0f; // Optional
	CreateInfoColorBlending.blendConstants[3] = 0.0f; // Optional

	// Dynamic state： A limited amount of the state that we've specified in the previous structs can 
	// actually be changed without recreating the pipeline.
	/*VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;*/

	// pipeline layout,相当于uniform变量的声明
	VkPipelineLayoutCreateInfo CreateInfoPipelineLayout = {};
	CreateInfoPipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	CreateInfoPipelineLayout.setLayoutCount = 0;
	CreateInfoPipelineLayout.pSetLayouts = nullptr; // Optional
	CreateInfoPipelineLayout.pushConstantRangeCount = 0; // Optional
	CreateInfoPipelineLayout.pPushConstantRanges = nullptr; // Optional

	Result = vkCreatePipelineLayout(m_Device, &CreateInfoPipelineLayout, nullptr, &m_PipelineLayout);
	if (Result != VK_SUCCESS) return Result;

	VkGraphicsPipelineCreateInfo CreateInfoPipeline = {};
	CreateInfoPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	CreateInfoPipeline.stageCount = 2;
	CreateInfoPipeline.pStages = ShaderStages;
	CreateInfoPipeline.pVertexInputState = &CreateInfoVertexInput;
	CreateInfoPipeline.pInputAssemblyState = &CreateInfoInputAssembly;
	CreateInfoPipeline.pViewportState = &CreateInfoViewportState;
	CreateInfoPipeline.pRasterizationState = &CreateInfoRasterizer;
	CreateInfoPipeline.pMultisampleState = &CreateInfoMultisample;
	CreateInfoPipeline.pDepthStencilState = &CreateInfoDepthStencil;
	CreateInfoPipeline.pColorBlendState = &CreateInfoColorBlending;
	CreateInfoPipeline.pDynamicState = nullptr; // Optional
	CreateInfoPipeline.layout = m_PipelineLayout;
	CreateInfoPipeline.renderPass = m_RenderPass;
	CreateInfoPipeline.subpass = 0;
	CreateInfoPipeline.basePipelineHandle = VK_NULL_HANDLE; // Optional
	CreateInfoPipeline.basePipelineIndex = -1; // Optional

	Result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &CreateInfoPipeline, nullptr, &m_GraphicsPipeline);
	if (Result != VK_SUCCESS) return Result;
	
	// shader module使用后即可销毁
	vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);

	return VK_SUCCESS;
}

/*******************************
* required: m_Device, m_SwapChainImageViews, m_RenderPass, m_SwapChainExtent
* assigned: m_SwapChainFramebuffers
*/
VkResult				CVulkanApp::__createFramebuffers() {
	outputTips("创建Frame buffer...");
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

	VkResult Result;
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		/*VkImageView Attachments[] = {
			m_SwapChainImageViews[i]
		};*/
		array<VkImageView, 1> Attachments = {
			m_SwapChainImageViews[i]
		};

		VkFramebufferCreateInfo FramebufferInfo = {};
		FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferInfo.renderPass = m_RenderPass;
		FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
		FramebufferInfo.pAttachments = Attachments.data();
		FramebufferInfo.width = m_SwapChainExtent.width;
		FramebufferInfo.height = m_SwapChainExtent.height;
		FramebufferInfo.layers = 1;

		Result = vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_SwapChainFramebuffers[i]);
		if (Result != VK_SUCCESS) return Result;
	}
	return VK_SUCCESS;
}

/*******************************
* required: m_PhysicalDevice, m_Device
* assigned: m_CommandPool
*/
VkResult				CVulkanApp::__createCommandPool() {
	// vulkan中的命令并不是调用函数直接执行，而是需要放到命令缓存对象里，这样的好处是命令可以预先执行或者多线程执行

	// 在创建命令缓存之前需要创建命令池，命令池用于管理内存、存储命令缓存，因此命令缓存的内存是由它分配的

	// 命令就像图像和显示一样，需要被提交到一个命令队列里，而所有的图形队列和显示队列都一定额外支持命令队列
	outputTips("创建Command pool...");
	SQueueFamilyIndices QueueFamilyIndices = __findQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo CreateInfoCommandPool = {};
	CreateInfoCommandPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfoCommandPool.queueFamilyIndex = QueueFamilyIndices.GraphicsFamily.value();

	return vkCreateCommandPool(m_Device, &CreateInfoCommandPool, nullptr, &m_CommandPool);
}

/*******************************
* required:
* assigned:
*/
VkResult				CVulkanApp::__createImageView(VkImageView& voImageView, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags) {
	// 为image创建image view

	VkImageViewCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	CreateInfo.image = vImage; // 对应的image
	CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // viewType和format表示了如何解释数据
	CreateInfo.format = vFormat;
	// subresourceRange选择view可访问的mipmap层数和数组层数
	CreateInfo.subresourceRange.aspectMask = vAspectFlags;						// ？方向特性
	// VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
	// VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
	// VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
	// VK_IMAGE_ASPECT_METADATA_BIT = 0x00000008,
	CreateInfo.subresourceRange.baseMipLevel = 0;
	CreateInfo.subresourceRange.levelCount = 1;
	CreateInfo.subresourceRange.baseArrayLayer = 0;
	CreateInfo.subresourceRange.layerCount = 1;

	return vkCreateImageView(m_Device, &CreateInfo, nullptr, &voImageView);
}

/*******************************
* required: m_Vertices, m_Device
* assigned: m_VertexBuffer, m_VertexBufferMemory
*/
void				CVulkanApp::__createVertexBuffer() {
	VkDeviceSize BufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingBufferMemory;
	__createBuffer(StagingBuffer, StagingBufferMemory, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* pData;
	vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, m_Vertices.data(), (size_t)BufferSize);
	vkUnmapMemory(m_Device, StagingBufferMemory);

	__createBuffer(m_VertexBuffer, m_VertexBufferMemory, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	__copyBuffer(StagingBuffer, m_VertexBuffer, BufferSize);

	vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
	vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

/*******************************
* required: m_Indices, m_Device
* assigned: m_IndexBuffer, m_IndexBufferMemory
*/
void				CVulkanApp::__createIndexBuffer() {
	VkDeviceSize BufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	VkBuffer StagingBuffer;
	VkDeviceMemory StagingBufferMemory;
	__createBuffer(StagingBuffer, StagingBufferMemory, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* pData;
	vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData);
	memcpy(pData, m_Indices.data(), (size_t)BufferSize);
	vkUnmapMemory(m_Device, StagingBufferMemory);

	__createBuffer(m_IndexBuffer, m_IndexBufferMemory, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	__copyBuffer(StagingBuffer, m_IndexBuffer, BufferSize);

	vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
	vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

/*******************************
* required: m_SwapChainFramebuffers, m_CommandPool, m_RenderPass, m_GraphicsPipeline, m_VertexBuffer, m_IndexBuffer, m_PipelineLayout
* assigned: m_CommandBuffers
*/
VkResult				CVulkanApp::__createCommandBuffers() {
	outputTips("创建Command buffer...");
	m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.commandPool = m_CommandPool;
	// The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
	// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
	// VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	VkResult Result;
	Result = vkAllocateCommandBuffers(m_Device, &AllocInfo, m_CommandBuffers.data());
	if (Result != VK_SUCCESS) return Result;

	for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
		// 开始填入命令
		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// flags指明一些功能
		// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: 执行一次后会重新填入命令
		// VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
		// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: 即使命令正在执行，它也可以重新被提交
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		BeginInfo.pInheritanceInfo = nullptr; // Optional

		Result = vkBeginCommandBuffer(m_CommandBuffers[i], &BeginInfo);
		if (Result != VK_SUCCESS) return Result;

		// 开启render pass意味着开始绘制
		VkRenderPassBeginInfo RenderPassInfo = {};
		RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassInfo.renderPass = m_RenderPass;
		RenderPassInfo.framebuffer = m_SwapChainFramebuffers[i];
		RenderPassInfo.renderArea.offset = { 0, 0 }; // 绘制区域大小
		RenderPassInfo.renderArea.extent = m_SwapChainExtent;

		VkClearValue ClearColor = {0.3f, 0.3f, 0.3f, 1.0f}; // 背景颜色
		RenderPassInfo.clearValueCount = 1;
		RenderPassInfo.pClearValues = &ClearColor;

		// 开始录入命令，所有vkCmd前缀的函数都是录入命令的函数，他们的返回值都是void
		vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		VkDeviceSize Offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, &m_VertexBuffer, Offsets);
		vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		//vkCmdDraw(m_CommandBuffers[i], static_cast<uint32_t>(m_Vertices.size()), 1, 0, 0);
		vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_CommandBuffers[i]);
		Result = vkEndCommandBuffer(m_CommandBuffers[i]);
		if (Result != VK_SUCCESS) return Result;
	}

	return VK_SUCCESS;
}

/*******************************
* required: 
* assigned: m_ImageAvailableSemaphores, m_RenderFinishedSemaphores, m_InFlightFences
*/
VkResult				CVulkanApp::__createSemaphores() {
	outputTips("创建信号量...");
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo SemaphoreInfo = {};
	SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo FenceInfo = {};
	FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult Result;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		Result = vkCreateSemaphore(m_Device, &SemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]);
		if (Result != VK_SUCCESS) return Result;

		Result = vkCreateSemaphore(m_Device, &SemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]);
		if (Result != VK_SUCCESS) return Result;

		Result = vkCreateFence(m_Device, &FenceInfo, nullptr, &m_InFlightFences[i]);
		if (Result != VK_SUCCESS) return Result;
	}

	return VK_SUCCESS;
}

/*******************************
* required:
* assigned:
*/
void					CVulkanApp::__cleanupSwapChain() {
	for (auto framebuffer : m_SwapChainFramebuffers) {
		vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	for (auto imageView : m_SwapChainImageViews) {
		vkDestroyImageView(m_Device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
}

/*******************************
* 
*/
void					CVulkanApp::__recreateSwapChain() {
	outputTips("重建Swap Chain...");
	vkDeviceWaitIdle(m_Device);

	__cleanupSwapChain();

	__createSwapChain();
	__createImageViews();
	__createRenderPass();
	__createGraphicsPipeline();
	__createFramebuffers();
	__createCommandBuffers();
}


/*******************************
* required: m_ValidationLayers
*/
bool					CVulkanApp::__isValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// 遍历判断是否支持，对每个开启layer都在支持的layer里找有没有相同的，没有说明不支持
	for (const char* layerName : m_ValidationLayers) {
		bool LayerFound = false;
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				LayerFound = true;
				break;
			}
		}
		if (!LayerFound) {
			return false;
		}
	}

	return true;
}

/*******************************
* required: m_Surface, m_DeviceExtensions
*/
bool					CVulkanApp::__isDeviceSuitable(VkPhysicalDevice vDevice)
{
	SQueueFamilyIndices Indices = __findQueueFamilies(vDevice);

	bool ExtensionsSupported = __isDeviceExtensionSupport(vDevice);

	bool SwapChainAdequate = false;
	if (ExtensionsSupported) {
		SSwapChainSupportDetails SwapChainSupport = __querySwapChainSupport(vDevice);
		SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures SupportedFeatures;
	vkGetPhysicalDeviceFeatures(vDevice, &SupportedFeatures);

	return Indices.isComplete() && ExtensionsSupported && SwapChainAdequate && SupportedFeatures.samplerAnisotropy;
}

/*******************************
* required: m_Surface
*/
SQueueFamilyIndices		CVulkanApp::__findQueueFamilies(const VkPhysicalDevice& vDevice)
{
	// 获取合适的队列族
	SQueueFamilyIndices Indices;

	// 获取queuefamily
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

	vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

	// 选择合适的队列族
	for (int i = 0; i < QueueFamilies.size(); i++) {
		const auto& QueueFamily = QueueFamilies[i];

		if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {	// 是否支持图像命令
			Indices.GraphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vDevice, i, m_Surface, &presentSupport);

		if (QueueFamily.queueCount > 0 && presentSupport) {									// 是否支持显示
			Indices.PresentFamily = i;
		}
		if (Indices.isComplete()) {
			break;
		}
	}
	return Indices;
}

/*******************************
* required: m_DeviceExtensions
*/
bool					CVulkanApp::__isDeviceExtensionSupport(VkPhysicalDevice vDevice)
{
	uint32_t ExtensionCount;
	vkEnumerateDeviceExtensionProperties(vDevice, nullptr, &ExtensionCount, nullptr);

	vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
	vkEnumerateDeviceExtensionProperties(vDevice, nullptr, &ExtensionCount, AvailableExtensions.data());

	set<string> RequiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for (const auto& extension : AvailableExtensions) {
		RequiredExtensions.erase(extension.extensionName);
	}

	return RequiredExtensions.empty();
}

/*******************************
* required: m_Surface
*/
SSwapChainSupportDetails CVulkanApp::__querySwapChainSupport(VkPhysicalDevice vDevice)
{
	SSwapChainSupportDetails SwapChainSupport;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vDevice, m_Surface, &SwapChainSupport.Capabilities);

	uint32_t FormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vDevice, m_Surface, &FormatCount, nullptr);

	if (FormatCount != 0) {
		SwapChainSupport.Formats.resize(FormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vDevice, m_Surface, &FormatCount, SwapChainSupport.Formats.data());
	}

	uint32_t PresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vDevice, m_Surface, &PresentModeCount, nullptr);

	if (PresentModeCount != 0) {
		SwapChainSupport.PresentModes.resize(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(vDevice, m_Surface, &PresentModeCount, SwapChainSupport.PresentModes.data());
	}

	return SwapChainSupport;
}

/*******************************
* required:
* assigned:
*/
VkSurfaceFormatKHR		CVulkanApp::__chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& vAvailableFormats) {
	if (vAvailableFormats.size() == 1 && vAvailableFormats[0].format == VK_FORMAT_UNDEFINED) {		// 最好的情况，surface没有倾向的格式，可以任意选择
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : vAvailableFormats) {											// 否则我们看是否有我们所需的
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	// 如果都没有，则需要比较使用哪个格式最好，但一般返回第一个就可以了
	return vAvailableFormats[0];
}

/*******************************
* required:
* assigned:
*/
VkPresentModeKHR		CVulkanApp::__chooseSwapPresentMode(const vector<VkPresentModeKHR>& vAvailablePresentModes) {
	// VK_PRESENT_MODE_MAILBOX_KHR>VK_PRESENT_MODE_IMMEDIATE_KHR>VK_PRESENT_MODE_FIFO_KHR 
	VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;											// 一定支持的模式

	for (const auto& availablePresentMode : vAvailablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {									// triple buffering
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			BestMode = availablePresentMode;
		}
	}

	return BestMode;
}

VkExtent2D				CVulkanApp::__chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities) {
	// swap extent相当于绘制区域，大部分情况下和窗口大小相同

	// currentExtent是当前suface的长宽，如果是(0xFFFFFFFF, 0xFFFFFFFF)说明长宽会有对应的swapchain的extent决定
	if (vCapabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
		return vCapabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(m_pWindow, &width, &height); // 获取窗口的长宽

		VkExtent2D ActualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		return ActualExtent;
	}
}

void				CVulkanApp::__getPhysicalDeviceInfo(VkPhysicalDeviceProperties& voPhysicalDeviceProp, VkPhysicalDeviceFeatures& voPhysicalDeviceFeature, VkPhysicalDeviceMemoryProperties& voPhysicalDeviceMemProp) {
	outputTips("获取Physical Device相关信息...");

	// 获取物理设备各种信息

	// 获取物理设备属性（Properties）
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &voPhysicalDeviceProp);						// 获取GTX的属性

	// cout << physicalDeviceProp->deviceName << endl;											// 打印该设备名称

	// 获取物理设备特性（Feature）
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &voPhysicalDeviceFeature);					// 获取GTX的特性

	// 获取可用的extensions并剔除不可用的
	__isDeviceExtensionSupport(m_PhysicalDevice);

	// 获取物理设备内存
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &voPhysicalDeviceMemProp);			// 获取GTX内存属性
}

/*******************************
* required:
* assigned:
*/
uint32_t				CVulkanApp::__findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

/*******************************
* required:
* assigned:
*/
void					CVulkanApp::__copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = __beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	__endSingleTimeCommands(commandBuffer);

}

/*******************************
* required:
* assigned:
*/
VkCommandBuffer			CVulkanApp::__beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocInfo.commandPool = m_CommandPool;
	AllocInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	vkAllocateCommandBuffers(m_Device, &AllocInfo, &CommandBuffer);

	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

	return CommandBuffer;
}

/*******************************
* required:
* assigned:
*/
void					CVulkanApp::__endSingleTimeCommands(VkCommandBuffer vCommandBuffer) {
	vkEndCommandBuffer(vCommandBuffer);

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &vCommandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &vCommandBuffer);
}

/*******************************
* required: m_Device
* assigned: m_CommandPool
*/
VkResult					CVulkanApp::__createBuffer(VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties) {
	VkBufferCreateInfo BufferInfo = {};
	BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferInfo.size = vSize;
	BufferInfo.usage = vUsage;
	BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult Result;
	Result = vkCreateBuffer(m_Device, &BufferInfo, nullptr, &voBuffer);
	if (Result != VK_SUCCESS) return Result;

	VkMemoryRequirements MemRequirements;
	vkGetBufferMemoryRequirements(m_Device, voBuffer, &MemRequirements);

	VkMemoryAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	AllocInfo.allocationSize = MemRequirements.size;
	AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

	Result = vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voBufferMemory);
	if (Result != VK_SUCCESS) return Result;

	Result = vkBindBufferMemory(m_Device, voBuffer, voBufferMemory, 0);
	if (Result != VK_SUCCESS) return Result;

	return VK_SUCCESS;
}

/*******************************
* required:
* assigned:
*/
VkFormat				CVulkanApp::__findSupportedFormat(const vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures) {
	for (VkFormat format : vCandidates) {
		VkFormatProperties Props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &Props);

		if (vTiling == VK_IMAGE_TILING_LINEAR && (Props.linearTilingFeatures & vFeatures) == vFeatures) {
			return format;
		}
		else if (vTiling == VK_IMAGE_TILING_OPTIMAL && (Props.optimalTilingFeatures & vFeatures) == vFeatures) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

/*******************************
* 
*/
VkFormat				CVulkanApp::__findDepthFormat() {
	return __findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

#include <fstream>
/*******************************
* 
*/
vector<char>			CVulkanApp::readFile(const string& vFilename) {
	std::ifstream File(vFilename, std::ios::ate | std::ios::binary);

	if (!File.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t FileSize = (size_t)File.tellg();
	vector<char> Buffer(FileSize);
	File.seekg(0);
	File.read(Buffer.data(), FileSize);
	File.close();

	cout << "读取文件：" << vFilename << " 长度：" << FileSize << endl;
	return Buffer;
}