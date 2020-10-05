#include "VulkanApp.h"

#include <iostream>
using std::cout;
using std::endl;
using std::cerr;
using std::numeric_limits;

inline void outputTips(string text)
{
	// ���������ڷ������ʽ�����
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
	// vulkan�ĳ�ʼ�����൱��main����
	outputTips("------------------------------Vulkan��ʼ��------------------------------");

	//��ʼ��Vulkan
	_ASSERT(__createInstance(vExtensions) == VK_SUCCESS); // ����ʵ��
#if _DEBUG
	_ASSERT(__setupDebugMessenger() == VK_SUCCESS); // ����debug messenger
#endif
	_ASSERT(__createSurface() == VK_SUCCESS); // ����surface
	_ASSERT(__choosePhysicalDevice() == VK_SUCCESS); // ��ȡ�����豸

	_ASSERT(__createDevice() == VK_SUCCESS); // �����߼��豸

	_ASSERT(__createSwapChain() == VK_SUCCESS); // ����swap chain
	_ASSERT(__createImageViews() == VK_SUCCESS); // ����image view
	_ASSERT(__createRenderPass() == VK_SUCCESS); // ����Render pass

	//_ASSERT(__createDescriptorSetLayout() == VK_SUCCESS); // ����Descriptor Set Layout
	_ASSERT(__createGraphicsPipeline() == VK_SUCCESS); // ��������

	_ASSERT(__createFramebuffers() == VK_SUCCESS); // ����֡����
	_ASSERT(__createCommandPool() == VK_SUCCESS); // ���������
	__createVertexBuffer(); // �������㻺��
	__createIndexBuffer(); // ������������
	//_ASSERT(__createDescriptorPool() == VK_SUCCESS); // ������������
	//_ASSERT(__createDescriptorSets() == VK_SUCCESS); // ������������

	_ASSERT(__createCommandBuffers() == VK_SUCCESS); // ���������
	_ASSERT(__createSemaphores() == VK_SUCCESS); // �����ź���
}

/*******************************
* assigned: m_pWindow
*/
void					CVulkanApp::initWindow() {
	outputTips("------------------------------���ڳ�ʼ��------------------------------");

	// ��ʼ����ʾ����
	glfwInit();																						// ��ʼ��GLFW��

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);													// GLFWĬ�ϴ���OpenGL�������Ǹ�������Ҫ����OpenGL
	m_pWindow = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);					// ��������

	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetFramebufferSizeCallback(m_pWindow, framebufferResizeCallback);
	return;
}

/*******************************
* 
*/
void					CVulkanApp::drawFrame() {
	// drawFrame����ɣ��ӽ�������ȡͼ����ͼ����ִ���������������ͼ�����������׼����ʾ

	// ��Щ����ʵ�����첽ִ�еģ�����Ⱥ�˳����δ֪�ģ������ǵĳ�����ͬ���ģ���һ��������һ���Ľ������ߵ�����ͬ�����⡣
	// �����ַ�����ͬ����Щ�¼���fences and semaphores �����Ƕ�����һ���¼�����signaled״̬���������¼�����unsignaled״̬
	// �������ڣ�fences����vkWaitForFences�����ĺ���������ʹ�ã���semaphores����

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
	outputTips("-------------------------------������ѭ��-------------------------------");
	while (!glfwWindowShouldClose(m_pWindow)) {
		glfwPollEvents();							// ��������¼�����ѯ�͵ȴ����֣���Ϊ��Ϸÿ֡һ��Ⱦ��ʹ����ѯ����
		drawFrame();
	}
	vkDeviceWaitIdle(m_Device); // �ȵ�����ͬ������������ɺ��ٹر�
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
	vkDestroyDevice(m_Device, nullptr);					// �����߼��豸
	if (_DEBUG) {
		__destroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);	// ����surface��GLFWû������surface�ĺ�������vulkan��
	vkDestroyInstance(m_Instance, nullptr);				// ����vulkanʵ��
	glfwDestroyWindow(m_pWindow);							// ����GLFW����
	glfwTerminate();									// �ر�GLFW
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
	// ʵ����instance���ǽ���Ӧ����vulkan��֮����ϵ�Ķ���
	outputTips("����ʵ��...");
	// �󲿷�vulkan����Ĵ�������ѭ�����Ĺ����½�һ����Ӧ����Ȼ��ʹ��һ��info���������������ԣ�����䴫����Ӧ�ĺ���
	VkApplicationInfo InfoApp = { };														// Ӧ�ó�����Ϣ������instance������
	InfoApp.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;		// sType�Ǵ󲿷�info�ṹ���е����ԣ��������ֲ�ͬinfo������
	InfoApp.pApplicationName = "Hello Triangle";							// Ӧ�ó������ƣ������
	InfoApp.applicationVersion = 1;										// Ӧ�ó���汾�������
	InfoApp.apiVersion = VK_MAKE_VERSION(1, 0, 0);					// Ӧ�ó���������������vulkan�汾������ֱ���õ�ǰ�汾����

	// ����ʵ��
	VkInstanceCreateInfo CreateInfoInstance = { };										// ʵ��������Ϣ
	CreateInfoInstance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	// ͬappinfo
	CreateInfoInstance.pApplicationInfo = &InfoApp;									// Ӧ�ó�����Ϣ

#if _DEBUG
	_ASSERT(__isValidationLayerSupport());
	// ����layers
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

	return vkCreateInstance(&CreateInfoInstance, nullptr, &m_Instance);						// ����ʵ��
}

/*******************************
* 
*/
VkResult				CVulkanApp::__createDebugUtilsMessengerEXT(VkInstance vInstance, const VkDebugUtilsMessengerCreateInfoEXT* vpCreateInfo, const VkAllocationCallbacks* vpAllocator, VkDebugUtilsMessengerEXT* vpDebugMessenger) {
	auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT");
	// vkGetInstanceProcAddr���ض�Ӧ�������ĺ���ָ�룬���ص�����ΪPFN_vkVoidFunction����Ҫ�Լ�ת��
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
	outputTips("����������...");
	VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	// messageSeverity��ʾ�ᴥ���ص���������Ϣ�ȼ�
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT �й�Vulkan loader��layers��drivers�����������Ϣ���ᱻ��ȡ
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ������Դ����֮��ı���ʽ����Ϣ�ᱻ��ȡ
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT warning��Ϣ�����ܵ���bug��Ҳ����ֻ��Ҫ����
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT error��Ϣ
	CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	// messageType��ʾ�ᴥ���ص�����������
	// VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT specifies that some general event has occurred. This is typically a non-specification, non-performance event.
	// VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT specifies that something has occurred during validation against the Vulkan specification that may indicate invalid behavior.
	// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT specifies a potentially non-optimal use of Vulkan, e.g. using vkCmdClearColorImage when setting VkAttachmentDescription::loadOp to VK_ATTACHMENT_LOAD_OP_CLEAR would have worked.
	CreateInfo.pfnUserCallback = debugCallback;				// ��Ӧ�Ļص�������
	CreateInfo.pUserData = nullptr;						// ��Ӧ�ص���������Ĳ�����Ĭ��Ϊ��

	return __createDebugUtilsMessengerEXT(m_Instance, &CreateInfo, nullptr, &m_DebugMessenger);
}

/*******************************
* required: m_Instance, m_pWindow
* assigned: m_Surface
*/
VkResult				CVulkanApp::__createSurface() {
	// surface�������Ϊ���棬��һ��չʾͼ�������
	outputTips("����Surface...");

	// ����surface
	return glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface);
}

/*******************************
* required: m_Instance
* assigned: m_PhysicalDevices, m_PhysicalDevice
*/
VkResult				CVulkanApp::__choosePhysicalDevice() {
	// ��ȡ�����豸
	outputTips("��ȡPysical Device...");

	// �����豸����ʵ���ڵ����Կ���CPU��Ӳ��
	uint32_t PhysicalDeviceCount = 0;
	VkResult Result = vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, nullptr);	// ��ȡ�����豸��Physical Device������
	if (Result == VK_SUCCESS) {
		m_PhysicalDevices.resize(PhysicalDeviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, m_PhysicalDevices.data()); // ��ȡ�����豸��Physical Device��
	}
	else return Result;

	// ѡ����ʵ������豸
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
	outputTips("�����߼��豸...");

	VkPhysicalDeviceProperties PhysicalDeviceProp; // �����豸����
	VkPhysicalDeviceFeatures PhysicalDeviceFeature; // �����豸����
	VkPhysicalDeviceMemoryProperties  PhysicalDeviceMemProp; // �����豸�ڴ�����
	__getPhysicalDeviceInfo(PhysicalDeviceProp, PhysicalDeviceFeature, PhysicalDeviceMemProp); // ��ȡ�����豸��Ϣ

	SQueueFamilyIndices Indices = __findQueueFamilies(m_PhysicalDevice);

	vector<VkDeviceQueueCreateInfo> CreateInfosQueue;
	set<uint32_t> UniqueQueueFamilies = { Indices.GraphicsFamily.value(), Indices.PresentFamily.value() };

	float QueuePriority = 1.0f;
	for (uint32_t queueFamily : UniqueQueueFamilies) {
		VkDeviceQueueCreateInfo CreateInfoQueue = {};
		CreateInfoQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		CreateInfoQueue.queueFamilyIndex = queueFamily;
		CreateInfoQueue.queueCount = 1;
		CreateInfoQueue.pQueuePriorities = &QueuePriority; // �������ȼ�����ʹֻ��һ��queueҲ��Ҫ��д
		CreateInfosQueue.push_back(CreateInfoQueue);
	}

	// �����߼��豸
	VkPhysicalDeviceFeatures RequiredFeatures = {}; // ���������

	RequiredFeatures.multiDrawIndirect = PhysicalDeviceFeature.multiDrawIndirect;
	RequiredFeatures.tessellationShader = VK_TRUE;
	RequiredFeatures.geometryShader = VK_TRUE;
	RequiredFeatures.samplerAnisotropy = VK_TRUE; // ���������˲���

	VkDeviceCreateInfo CreateInfoDevice = {};
	CreateInfoDevice.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	CreateInfoDevice.queueCreateInfoCount = static_cast<uint32_t>(CreateInfosQueue.size());
	CreateInfoDevice.pQueueCreateInfos = CreateInfosQueue.data();
	CreateInfoDevice.pEnabledFeatures = &RequiredFeatures;
	// ע�⣺�°��vulkan�Ѿ�û��instance��device��layer��extension�������ˣ�device����Щ���Իᱻ���ԣ���Ϊ�����¼��ݻ��ǿ�������
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
	// swap chain������֡���棬һ�����Կ��ڴ棬�����ȶ�֡������;
	// swap chain����������buffer��һ��Ϊscreenbuffer��������Ⱦ���������һ����backbuffers��ÿ����ʾһ֡ʱ��backbuffer�е����ݻ��滻screenbuffer��������̽���presentation����swaping
	outputTips("����Swap Chain...");

	// ʹ��swap chain֮ǰ��Ҫ��ѯ����Ϣ
	SSwapChainSupportDetails SwapChainSupport
	= __querySwapChainSupport(m_PhysicalDevice);

	uint32_t PresentModesCount = static_cast<uint32_t>(SwapChainSupport.PresentModes.size());
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &PresentModesCount, SwapChainSupport.PresentModes.data());
	// VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.
	// VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue. If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank".
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
	// VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to implement triple buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync that uses double buffering.

	// ����swap chain
	VkSurfaceFormatKHR SurfaceFormat = __chooseSwapSurfaceFormat(SwapChainSupport.Formats); // ѡ��surface��ʽ
	VkPresentModeKHR PresentMode = __chooseSwapPresentMode(SwapChainSupport.PresentModes); // ѡ����ʾģʽ
	VkExtent2D Extent = __chooseSwapExtent(SwapChainSupport.Capabilities); // ����swapchain�����С


	// imageCount�����ж���images��swap chain�����ʹ���������Сֵ
	// �����ֻ����Сֵ��ζ��sometimes have to wait on the driver to complete internal operations before we can acquire another image to render to. ����+1

	uint32_t ImageCount = SwapChainSupport.Capabilities.minImageCount + 1;

	// ����Ҫȷ�����ᳬ��max��max>=min����maxΪ0��ζ�������ޣ����ų�����
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
	CreateInfoSwapChain.preTransform = SwapChainSupport.Capabilities.currentTransform;	// �����任����ת������
	CreateInfoSwapChain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// alphaͨ��ģʽ����ģʽ����alpha����Ĭ��alpha��Ϊ1.0
	CreateInfoSwapChain.presentMode = PresentMode;
	CreateInfoSwapChain.clipped = VK_TRUE;											// �Ƿ����öԲ��ɼ�ͼ�񲿷ֵ��������
	CreateInfoSwapChain.oldSwapchain = VK_NULL_HANDLE;									// �� �ɰ�����Դ����

	VkResult Result = vkCreateSwapchainKHR(m_Device, &CreateInfoSwapChain, nullptr, &m_SwapChain);
	if (Result != VK_SUCCESS) return Result;

	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, nullptr);
	m_SwapChainImages.resize(ImageCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &ImageCount, m_SwapChainImages.data()); // ��ȡ��swap chain��image���洢

	m_SwapChainImageFormat = SurfaceFormat.format; // ����ͼ���ʽ
	m_SwapChainExtent = Extent; // ����������Ϣ
	
	return VK_SUCCESS;
}

/*******************************
* required: m_SwapChainImages
* assigned: m_SwapChainImageViews, m_SwapChainImageFormat, m_SwapChainExtent
*/
VkResult				CVulkanApp::__createImageViews() {
	outputTips("����Image View...");

	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	// Ϊÿһ��image������Ӧ��imageview
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
	// ������ɫ��ģ��
	VkShaderModuleCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	CreateInfo.codeSize = vCode.size(); // ��ɫ�������С
	CreateInfo.pCode = reinterpret_cast<const uint32_t*>(vCode.data()); // ��ɫ�����룬reinterpret_castǿ������ת��

	return vkCreateShaderModule(m_Device, &CreateInfo, nullptr, &voShaderModule);
}

/*******************************
* required: m_Device, m_SwapChainImageFormat
* assigned: m_RenderPass
*/
VkResult				CVulkanApp::__createRenderPass() {
	// render pass����vulkan����Щ֡���渽����framebuffers attachment���ᱻ������Ⱦ

	outputTips("����Render pass...");
	VkAttachmentDescription ColorAttachment = {};
	ColorAttachment.format = m_SwapChainImageFormat;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// loadOp��storeOp��������Ⱦǰ����Ⱦ��Ը����Ĳ���
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
	// ͼ�ι��ߣ�����Ⱦ���ߣ�����ģ�͵���ʾ��֮��Ĵ������̣��漰��Ӳ���Ĵ���
	// ������->���㴦������������ɫ��������դ�����������ӣ�3Dת2D������Ķ���ת�����أ���ƬԪ��������ƬԪ��ɫ�����������֡���棬��Щ���������ǲ��е�
	outputTips("����ͼ�����...");

	// ��ȡ��ɫ����vulkan��ɫ��Ҫ��SPIR-V��ʽ������ʹ��GLSL�ɶ����Ա�д��ʹ��glslangValidator.exe����תΪSPIR-V��ʽ
	auto VertShaderCode = readFile("shader/vert.spv");
	auto FragShaderCode = readFile("shader/frag.spv");

	// Ҫ����ɫ�����䵽�������Ҫ�������VkShaderModule������
	VkShaderModule VertShaderModule, FragShaderModule;
	VkResult Result;
	Result = __createShaderModule(VertShaderModule, VertShaderCode);
	if (Result != VK_SUCCESS) return Result;
	Result = __createShaderModule(FragShaderModule, FragShaderCode);
	if (Result != VK_SUCCESS) return Result;

	// Ҫʹ����ɫ������Ҫ������䵽�����ϲ���
	VkPipelineShaderStageCreateInfo CreateInfoVertShaderStage = {};
	CreateInfoVertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	CreateInfoVertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	CreateInfoVertShaderStage.module = VertShaderModule;
	CreateInfoVertShaderStage.pName = "main"; // ���������ƣ�һ��Ϊmain

	VkPipelineShaderStageCreateInfo CreateInfoFragShaderStage = {};
	CreateInfoFragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	CreateInfoFragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	CreateInfoFragShaderStage.module = FragShaderModule;
	CreateInfoFragShaderStage.pName = "main";

	VkPipelineShaderStageCreateInfo ShaderStages[] = { CreateInfoVertShaderStage, CreateInfoFragShaderStage };


	auto BindingDescription = SVertex::getBindingDescription();
	auto AttributeDescriptions = SVertex::getAttributeDescriptions();

	// VkPipelineVertexInputStateCreateInfo�����˴�����ɫ�������ݵĸ�ʽ
	VkPipelineVertexInputStateCreateInfo CreateInfoVertexInput = {};
	CreateInfoVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	CreateInfoVertexInput.vertexBindingDescriptionCount = 1;
	CreateInfoVertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeDescriptions.size());
	CreateInfoVertexInput.pVertexBindingDescriptions = &BindingDescription;
	CreateInfoVertexInput.pVertexAttributeDescriptions = AttributeDescriptions.data();

	// Input assemblyͼԪװ�� ,The VkPipelineInputAssemblyStateCreateInfo �����������£�����ģʽ���ߡ�������ȣ��Լ�ͼԪ����

	// primitive restartͼԪ������ https://segmentfault.com/a/1190000012271175

	VkPipelineInputAssemblyStateCreateInfo CreateInfoInputAssembly = {};
	CreateInfoInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	CreateInfoInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;				// ÿ������Ϊһ��������
	CreateInfoInputAssembly.primitiveRestartEnable = VK_FALSE;							// ͼԪ�����ر�

	// Viewports�������������� and scissors���ü���������������֡�����лᱻ��Ⱦ������һ���������֡����(0, 0)��(width, height)
	// ���ƿ��������൱�ڶ�1x1�����ƽ�����ţ����ü�����������ϵ�ɾ��
	// ���Կ��ƿ�߱�
	VkViewport Viewport = {};
	float Ratio = (float)m_SwapChainExtent.width / (float)m_SwapChainExtent.height;
	float CurrentWidth = (float)m_SwapChainExtent.width;
	float CurrentHeight = (float)m_SwapChainExtent.height;

	// �Զ���������
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

	// scissor�ü�ȷ�����ĸ������ڵ����ػᱻ����
	VkRect2D Scissor = {};
	Scissor.offset = { 0, 0 };
	Scissor.extent = m_SwapChainExtent;

	// �����Ͳü���Ҫ���ϵ�һ��viewport state��
	VkPipelineViewportStateCreateInfo CreateInfoViewportState = {};
	CreateInfoViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	CreateInfoViewportState.viewportCount = 1;
	CreateInfoViewportState.pViewports = &Viewport;
	CreateInfoViewportState.scissorCount = 1;
	CreateInfoViewportState.pScissors = &Scissor;

	// ��դ����3Dת2D���أ�ͬʱ��������Ȳ��ԡ������������ȣ�Ȼ�����������ƬԪ��ɫ��Ⱦɫ
	VkPipelineRasterizationStateCreateInfo CreateInfoRasterizer = {};
	CreateInfoRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// depthClamp��Ƚ�ȡ��ȡ��z��0-1�����ƣ��ڽ�ƽ��֮ǰ��Զƽ��֮��Ķ�����ʾ  https://blog.csdn.net/yangyong0717/article/details/78321968
	CreateInfoRasterizer.depthClampEnable = VK_FALSE;
	// ��դ��ǰ��������ͼԪ(�൱������հף���
	CreateInfoRasterizer.rasterizerDiscardEnable = VK_FALSE;
	// polygonMode����ģʽ
	// VK_POLYGON_MODE_FILL ���,һ��֧��
	// VK_POLYGON_MODE_LINE �߱�����Ϊʵ��,��Ҫ�Կ�֧��
	// VK_POLYGON_MODE_POINT �������ΪԲ��,��Ҫ�Կ�֧��
	CreateInfoRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	CreateInfoRasterizer.lineWidth = 1.0f;
	CreateInfoRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;				// ����������
	//CreateInfoRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//CreateInfoRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;		// �����ָ����ʽ,���ֶ���
	CreateInfoRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// ����shadow mapping,�������ǲ���
	CreateInfoRasterizer.depthBiasEnable = VK_FALSE;
	CreateInfoRasterizer.depthBiasConstantFactor = 0.0f; // Optional
	CreateInfoRasterizer.depthBiasClamp = 0.0f; // Optional
	CreateInfoRasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// MSAA�����
	VkPipelineMultisampleStateCreateInfo CreateInfoMultisample = {};
	CreateInfoMultisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	CreateInfoMultisample.sampleShadingEnable = VK_FALSE;
	CreateInfoMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	CreateInfoMultisample.minSampleShading = 1.0f; // Optional
	CreateInfoMultisample.pSampleMask = nullptr; // Optional
	CreateInfoMultisample.alphaToCoverageEnable = VK_FALSE; // Optional
	CreateInfoMultisample.alphaToOneEnable = VK_FALSE; // Optional

	// ��Ȳ��Ժ�ģ�����
	// ��Ȳ���ȷ����ǰ��˳��
	// ģ�����,��ģ�建����д�벻ͬ��ֵ,��ģ�����ʱ�����ֵ�ò�ͬ�������û��Ǳ�������
	VkPipelineDepthStencilStateCreateInfo CreateInfoDepthStencil = {};
	CreateInfoDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	CreateInfoDepthStencil.depthTestEnable = VK_TRUE;
	CreateInfoDepthStencil.depthWriteEnable = VK_TRUE;
	CreateInfoDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	CreateInfoDepthStencil.depthBoundsTestEnable = VK_FALSE;
	CreateInfoDepthStencil.stencilTestEnable = VK_FALSE;


	// ��ƬԪ��ɫ��������ɫ��,��Ҫ��֡�������е���ɫ���
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

	// �ڶ��ַ�ʽ,���ﲻ��
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

	// Dynamic state�� A limited amount of the state that we've specified in the previous structs can 
	// actually be changed without recreating the pipeline.
	/*VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;*/

	// pipeline layout,�൱��uniform����������
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
	
	// shader moduleʹ�ú󼴿�����
	vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);

	return VK_SUCCESS;
}

/*******************************
* required: m_Device, m_SwapChainImageViews, m_RenderPass, m_SwapChainExtent
* assigned: m_SwapChainFramebuffers
*/
VkResult				CVulkanApp::__createFramebuffers() {
	outputTips("����Frame buffer...");
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
	// vulkan�е�������ǵ��ú���ֱ��ִ�У�������Ҫ�ŵ���������������ĺô����������Ԥ��ִ�л��߶��߳�ִ��

	// �ڴ��������֮ǰ��Ҫ��������أ���������ڹ����ڴ桢�洢����棬����������ڴ������������

	// �������ͼ�����ʾһ������Ҫ���ύ��һ���������������е�ͼ�ζ��к���ʾ���ж�һ������֧���������
	outputTips("����Command pool...");
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
	// Ϊimage����image view

	VkImageViewCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	CreateInfo.image = vImage; // ��Ӧ��image
	CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // viewType��format��ʾ����ν�������
	CreateInfo.format = vFormat;
	// subresourceRangeѡ��view�ɷ��ʵ�mipmap�������������
	CreateInfo.subresourceRange.aspectMask = vAspectFlags;						// ����������
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
	outputTips("����Command buffer...");
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
		// ��ʼ��������
		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// flagsָ��һЩ����
		// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: ִ��һ�κ��������������
		// VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
		// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: ��ʹ��������ִ�У���Ҳ�������±��ύ
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		BeginInfo.pInheritanceInfo = nullptr; // Optional

		Result = vkBeginCommandBuffer(m_CommandBuffers[i], &BeginInfo);
		if (Result != VK_SUCCESS) return Result;

		// ����render pass��ζ�ſ�ʼ����
		VkRenderPassBeginInfo RenderPassInfo = {};
		RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassInfo.renderPass = m_RenderPass;
		RenderPassInfo.framebuffer = m_SwapChainFramebuffers[i];
		RenderPassInfo.renderArea.offset = { 0, 0 }; // ���������С
		RenderPassInfo.renderArea.extent = m_SwapChainExtent;

		VkClearValue ClearColor = {0.3f, 0.3f, 0.3f, 1.0f}; // ������ɫ
		RenderPassInfo.clearValueCount = 1;
		RenderPassInfo.pClearValues = &ClearColor;

		// ��ʼ¼���������vkCmdǰ׺�ĺ�������¼������ĺ��������ǵķ���ֵ����void
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
	outputTips("�����ź���...");
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
	outputTips("�ؽ�Swap Chain...");
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

	// �����ж��Ƿ�֧�֣���ÿ������layer����֧�ֵ�layer������û����ͬ�ģ�û��˵����֧��
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
	// ��ȡ���ʵĶ�����
	SQueueFamilyIndices Indices;

	// ��ȡqueuefamily
	uint32_t QueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, nullptr);

	vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vDevice, &QueueFamilyCount, QueueFamilies.data());

	// ѡ����ʵĶ�����
	for (int i = 0; i < QueueFamilies.size(); i++) {
		const auto& QueueFamily = QueueFamilies[i];

		if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {	// �Ƿ�֧��ͼ������
			Indices.GraphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vDevice, i, m_Surface, &presentSupport);

		if (QueueFamily.queueCount > 0 && presentSupport) {									// �Ƿ�֧����ʾ
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
	if (vAvailableFormats.size() == 1 && vAvailableFormats[0].format == VK_FORMAT_UNDEFINED) {		// ��õ������surfaceû������ĸ�ʽ����������ѡ��
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : vAvailableFormats) {											// �������ǿ��Ƿ������������
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	// �����û�У�����Ҫ�Ƚ�ʹ���ĸ���ʽ��ã���һ�㷵�ص�һ���Ϳ�����
	return vAvailableFormats[0];
}

/*******************************
* required:
* assigned:
*/
VkPresentModeKHR		CVulkanApp::__chooseSwapPresentMode(const vector<VkPresentModeKHR>& vAvailablePresentModes) {
	// VK_PRESENT_MODE_MAILBOX_KHR>VK_PRESENT_MODE_IMMEDIATE_KHR>VK_PRESENT_MODE_FIFO_KHR 
	VkPresentModeKHR BestMode = VK_PRESENT_MODE_FIFO_KHR;											// һ��֧�ֵ�ģʽ

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
	// swap extent�൱�ڻ������򣬴󲿷�����ºʹ��ڴ�С��ͬ

	// currentExtent�ǵ�ǰsuface�ĳ��������(0xFFFFFFFF, 0xFFFFFFFF)˵��������ж�Ӧ��swapchain��extent����
	if (vCapabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
		return vCapabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(m_pWindow, &width, &height); // ��ȡ���ڵĳ���

		VkExtent2D ActualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		return ActualExtent;
	}
}

void				CVulkanApp::__getPhysicalDeviceInfo(VkPhysicalDeviceProperties& voPhysicalDeviceProp, VkPhysicalDeviceFeatures& voPhysicalDeviceFeature, VkPhysicalDeviceMemoryProperties& voPhysicalDeviceMemProp) {
	outputTips("��ȡPhysical Device�����Ϣ...");

	// ��ȡ�����豸������Ϣ

	// ��ȡ�����豸���ԣ�Properties��
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &voPhysicalDeviceProp);						// ��ȡGTX������

	// cout << physicalDeviceProp->deviceName << endl;											// ��ӡ���豸����

	// ��ȡ�����豸���ԣ�Feature��
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &voPhysicalDeviceFeature);					// ��ȡGTX������

	// ��ȡ���õ�extensions���޳������õ�
	__isDeviceExtensionSupport(m_PhysicalDevice);

	// ��ȡ�����豸�ڴ�
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &voPhysicalDeviceMemProp);			// ��ȡGTX�ڴ�����
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

	cout << "��ȡ�ļ���" << vFilename << " ���ȣ�" << FileSize << endl;
	return Buffer;
}