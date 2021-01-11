#pragma once
#include "Camera.h"

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
	SLOW = 0x0020,
};

class CInteractor
{
public:
	CInteractor(GLFWwindow* vpWindow);

	void bindEvent();
	void enable() { m_Enabled = true; }
	void disable() { m_Enabled = false; }
	void update();
	void reset();

	float getSpeed() { return m_Speed; }
	std::shared_ptr<CCamera> getCamera() { return m_pCamera; }

	void setSpeed(float vSpeed) { m_Speed = vSpeed; }

	static void onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods);
	static void onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY);
	static void onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods);
	
private:
	float __getDeltaTime();

	GLFWwindow* m_pWindow = nullptr;

	std::shared_ptr<CCamera> m_pCamera = nullptr;
	float m_Speed = 3.0;
	const float m_BoostScale = 3.0;
	const float m_CrawlScale = 0.5;
	const float m_HorizontalSensetivity = 0.1;
	const float m_VerticalSensetivity = 0.1;

	bool m_Enabled = true;
	int m_MoveState = EMoveState::STOP;
	double m_LastMousePosX = 0.0;
	double m_LastMousePosY = 0.0;
	bool m_IsMoving = false;
	double m_LastPhi = 0.0; 
	double m_LastTheta = 0.0;
};

