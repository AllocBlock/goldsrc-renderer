#pragma once
#include "Pointer.h"
#include "Camera.h"
#include "DrawableUI.h"
#include "Timer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <map>
#include <functional>

/*
 * ����ָ��
 * ������Ĭ�Ϲرս��������������������취��
 * 1. ��ס�Ҽ�������������ģʽ���ɿ���������
 * 2. ����Cap Lock����������ģʽ���ٴΰ��½�������������ESCҲ�ɽ�������
 * ����ģʽ�»��������ָ��
 * WASD ǰ�������ƶ�
 * QE ���¡������ƶ�
 * �������Ҽ�ͷ ��ת�ӽ�
 * +- �ӿ졢�����ƶ�����ת�ٶ�
 */

using MouseCallback_t = std::function<void(GLFWwindow*, int, int)>;

class CInteractor : public IDrawableUI
{
public:
	CInteractor();

	void bindEvent(GLFWwindow* vWindow, CCamera::Ptr vCamera);
	void enable() { m_Enabled = true; m_Timer.start(); }
	void disable() { m_Enabled = false; }
	void update();
	void reset();

	void setMouseCallback(MouseCallback_t vCallback) { m_pMouseCallback = vCallback; }

	float getSpeed() { return m_Speed; }
	void setSpeed(float vSpeed) { m_Speed = vSpeed; }
	CCamera::Ptr getCamera() { return m_pCamera; }

	static void onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods);
	static void onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY);
	static void onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods);
	
private:
	virtual void _renderUIV() override;

	float __getDeltaTime();
	int __getCurrentMoveState();
	int __getCurrentRotateState();
	void __startMovingMode();
	void __stopMovingMode();
	void __updateMove(float vDeltaTime);
	void __updateRotate(float vDeltaTime);

	CTimer m_Timer;

	GLFWwindow* m_pWindow = nullptr;
	CCamera::Ptr m_pCamera = nullptr;

	float m_Speed = 3.0f;
	const float m_BoostScale = 3.0f;
	const float m_CrawlScale = 0.5f;
	const float m_HorizontalSensetivity = 0.1f;
	const float m_VerticalSensetivity = 0.1f;

	bool m_Enabled = true;
	bool m_IsMoving = false;
	float m_LastMousePosX = 0.0f;
	float m_LastMousePosY = 0.0f;
	float m_LastPhi = 0.0f; 
	float m_LastTheta = 0.0f;

	std::map<int, std::function<void()>> m_KeyCallbackMap;
	MouseCallback_t m_pMouseCallback = nullptr;
};

