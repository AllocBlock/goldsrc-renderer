#include "VulkanApp.h"
#include <GLFW/glfw3.h>

int main() {
	CVulkanApp App;
	App.initWindow();
	uint32_t ExtensionsCount = 0;
	const char** ppExtension = glfwGetRequiredInstanceExtensions(&ExtensionsCount);

	vector<const char*> Extensions;
	for (uint32_t i = 0; i < ExtensionsCount; i++)
		Extensions.emplace_back(ppExtension[i]);

	App.initVulkan(Extensions);
	App.mainLoop();
	App.cleanup();
	return 0;
}