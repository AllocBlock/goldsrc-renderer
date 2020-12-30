#include "Interactor.h"
#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include "IOGoldSrcMap.h"

int main()
{
	CIOGoldSrcMap Map("../data/test.map");
	Map.read();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	CVulkanRenderer Renderer(pWindow);
	Renderer.getCamera()->setPos(glm::vec3(0.0f, 0.0f, 3.0f));
	Renderer.readData("../data/ball.obj");
	Renderer.init();

	CInteractor Interactor(&Renderer);
	Interactor.bindEvent();

	std::chrono::milliseconds LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		std::chrono::milliseconds CurrentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		float DeltaTime = static_cast<float>((CurrentTimeStamp - LastTimeStamp).count()) / 1000.0f;
		Interactor.update(DeltaTime);
		Renderer.render();
		LastTimeStamp = CurrentTimeStamp;
	}
	Renderer.waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	return 0;
}