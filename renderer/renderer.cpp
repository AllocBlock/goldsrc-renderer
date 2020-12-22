// renderer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <GLFW/glfw3.h>
#include "Camera.h"

#include "VulkanRenderer.h"

CCamera* g_pCamera = nullptr;
enum Move {
	MOVE_STOP = 0x0000,
	MOVE_FRONT = 0x0001,
	MOVE_BEHIND = 0x0002,
	MOVE_LEFT = 0x0004,
	MOVE_RIGHT = 0x0008,
	MOVE_BOOST = 0x0010,
	MOVE_SLOW = 0x0020,
};

int g_MoveState = Move::MOVE_STOP;
double g_MousePosX, g_MousePosY;
bool g_Moving = false;
double g_LastPhi, g_LastTheta;

void updateCamera();
void keyboardCallback(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods);
void mousemoveCallback(GLFWwindow* vpWindow, double vPosX, double vPosY);
void mouseclickCallback(GLFWwindow* vpWindow, int vButton, int vAction, int vMods);

int main()
{
	g_pCamera = new CCamera;
	g_pCamera->setPos(glm::vec3(0.0f, 0.0f, 3.0f));

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	glfwSetKeyCallback(pWindow, keyboardCallback);
	glfwSetCursorPosCallback(pWindow, mousemoveCallback);
	glfwSetMouseButtonCallback(pWindow, mouseclickCallback);
	
	CVulkanRenderer Renderer(pWindow, g_pCamera);
	Renderer.readData("../data/ball.obj");
	Renderer.init();
	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		Renderer.render();
		updateCamera();
	}
	Renderer.waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	return 0;
}


void keyboardCallback(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods)
{
	if (vKey == GLFW_KEY_W)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_FRONT;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_FRONT;
	}
	else if (vKey == GLFW_KEY_S)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_BEHIND;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_BEHIND;
	}
	else if (vKey == GLFW_KEY_A)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_LEFT;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_LEFT;
	}
	else  if (vKey == GLFW_KEY_D)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_RIGHT;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_RIGHT;
	}
	else  if (vKey == GLFW_KEY_LEFT_SHIFT)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_BOOST;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_BOOST;
	}
	else  if (vKey == GLFW_KEY_LEFT_CONTROL)
	{
		if (vAction == GLFW_PRESS) g_MoveState |= MOVE_SLOW;
		else if (vAction == GLFW_RELEASE) g_MoveState &= ~MOVE_SLOW;
	}
}

void mousemoveCallback(GLFWwindow* vpWindow, double vPosX, double vPosY)
{
	if (!g_Moving) return;
	float Sensitivity = 0.1;
	g_pCamera->setPhi(g_LastPhi + (g_MousePosX - vPosX) * Sensitivity);
	g_pCamera->setTheta(std::min(std::max(g_LastTheta - (g_MousePosY - vPosY) * Sensitivity, 1.0), 179.0));
}

void mouseclickCallback(GLFWwindow* vpWindow, int vButton, int vAction, int vMods)
{
	if (vButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (vAction == GLFW_PRESS)
		{
			g_Moving = true;
			glfwGetCursorPos(vpWindow, &g_MousePosX, &g_MousePosY);
			g_LastPhi = g_pCamera->getPhi();
			g_LastTheta = g_pCamera->getTheta();
		}
		else if (vAction == GLFW_RELEASE)
		{
			g_Moving = false;
		}
	}
}

void updateCamera()
{
	const float Step = 0.01;
	float Boost = 1.0;
	float MoveForward = 0.0, MoveLeft = 0.0;
	if (g_MoveState == MOVE_STOP) return;
	if (g_MoveState & MOVE_BOOST) Boost *= 2.0;
	if (g_MoveState & MOVE_SLOW) Boost *= 0.5;
	if (g_MoveState & MOVE_FRONT) MoveForward += 1;
	if (g_MoveState & MOVE_BEHIND) MoveForward -= 1;
	if (g_MoveState & MOVE_LEFT) MoveLeft += 1;
	if (g_MoveState & MOVE_RIGHT) MoveLeft -= 1;

	glm::vec3 Front = g_pCamera->getFront();
	glm::vec3 Left = g_pCamera->getLeft();
	glm::vec3 Move = (Front * MoveForward + Left * MoveLeft) * Step * Boost;
	g_pCamera->setPos(g_pCamera->getPos() + Move);
}

