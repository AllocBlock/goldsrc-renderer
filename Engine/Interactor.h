#pragma once
#include "Camera.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <map>
#include <functional>

/*
 * ����ָ��
 * ������Ĭ�Ϲرս��������������������취��
 * 1. ��ס���������������ɿ���������
 * 2. ����Cap Lock�������������ٴΰ��½�������������ESCҲ�ɽ�������
 * ����ģʽ�»��������ָ��
 * WASD ǰ�������ƶ�
 * QE ���¡������ƶ�
 * �������Ҽ�ͷ ��ת�ӽ�
 * +- �ӿ졢�����ƶ�����ת�ٶ�
 */

class CInteractor
{
public:
	CInteractor() = default;

	void bindEvent(GLFWwindow* vWindow, std::shared_ptr<CCamera> vCamera);
	void enable() { m_Enabled = true; }
	void disable() { m_Enabled = false; }
	void update();
	void reset();

	float getSpeed() { return m_Speed; }

	void setSpeed(float vSpeed) { m_Speed = vSpeed; }

	static void onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods);
	static void onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY);
	static void onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods);
	
private:
	float __getDeltaTime();
	int __getCurrentMoveState();
	int __getCurrentRotateState();
	void __startMovingMode();
	void __stopMovingMode();
	void __updateMove(float vDeltaTime);
	void __updateRotate(float vDeltaTime);

	GLFWwindow* m_pWindow = nullptr;
	std::shared_ptr<CCamera> m_pCamera = nullptr;

	float m_Speed = 3.0;
	const float m_BoostScale = 3.0f;
	const float m_CrawlScale = 0.5f;
	const float m_HorizontalSensetivity = 0.1f;
	const float m_VerticalSensetivity = 0.1f;

	bool m_Enabled = true;
	bool m_IsMoving = false;
	double m_LastMousePosX = 0.0;
	double m_LastMousePosY = 0.0;
	double m_LastPhi = 0.0; 
	double m_LastTheta = 0.0;

	std::map<int, std::function<void()>> m_KeyCallbackMap;
};

