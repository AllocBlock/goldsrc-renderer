#pragma once
#include "VulkanRenderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

enum EMoveState
{
	STOP = 0x0000,
	FRONT = 0x0001,
	BEHIND = 0x0002,
	LEFT = 0x0004,
	RIGHT = 0x0008,
	BOOST = 0x0010,
	CRAWL = 0x0020,
};

class CInteractor
{
public:
	CInteractor() = delete;
	CInteractor(GLFWwindow* vpWindow, std::shared_ptr<CVulkanRenderer> vpRenderer);

	void bindEvent();
	void enable() { m_Enabled = true; }
	void disable() { m_Enabled = false; }
	void update();
	void reset();

	float getSpeed() { return m_Speed; }
	std::shared_ptr<CVulkanRenderer> getRenderer() { return m_pRenderer; }
	bool getSelectionState() { return m_SelectionEnable; }

	void setSpeed(float vSpeed) { m_Speed = vSpeed; }
	void setSelectionState(bool vState) { m_SelectionEnable = vState; }

	static void onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods);
	static void onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY);
	static void onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods);
	
private:
	float __getDeltaTime();
	int __getCurrentMoveState();
	void __selectByClick(glm::vec2 vPos);

	GLFWwindow* m_pWindow = nullptr;
	std::shared_ptr<CVulkanRenderer> m_pRenderer = nullptr;

	float m_Speed = 3.0;
	const float m_BoostScale = 3.0;
	const float m_CrawlScale = 0.5;
	const float m_HorizontalSensetivity = 0.1;
	const float m_VerticalSensetivity = 0.1;

	bool m_Enabled = true;
	double m_LastMousePosX = 0.0;
	double m_LastMousePosY = 0.0;
	bool m_IsMoving = false;
	double m_LastPhi = 0.0; 
	double m_LastTheta = 0.0;

	bool m_SelectionEnable = true;
};

