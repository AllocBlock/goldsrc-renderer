#pragma once
#include "RendererScene.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

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

class CSceneInteractor
{
public:
	CSceneInteractor() = default;

	void bindEvent(GLFWwindow* vWindow);
	void enable() { m_Enabled = true; }
	void disable() { m_Enabled = false; }
	void update();
	void reset();

	float getSpeed() { return m_Speed; }
	void setRendererScene(std::shared_ptr<CRendererScene> vRenderer);
	std::shared_ptr<CRendererScene> getRendererScene() { return m_pRenderer; }
	bool getSelectionState() { return m_SelectionEnable; }

	void setSpeed(float vSpeed) { m_Speed = vSpeed; }
	void setSelectionState(bool vState) { m_SelectionEnable = vState; }

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
	void __selectByClick(glm::vec2 vPos);
	bool __getIntersectionOfRayAndBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, S3DBoundingBox vBB, float& voNearT, float& voFarT);

	GLFWwindow* m_pWindow = nullptr;
	std::shared_ptr<CRendererScene> m_pRenderer = nullptr;

	float m_Speed = 3.0;
	const float m_BoostScale = 3.0f;
	const float m_CrawlScale = 0.5f;
	const float m_HorizontalSensetivity = 0.1f;
	const float m_VerticalSensetivity = 0.1f;

	bool m_Enabled = true;
	double m_LastMousePosX = 0.0;
	double m_LastMousePosY = 0.0;
	bool m_IsMoving = false;
	double m_LastPhi = 0.0; 
	double m_LastTheta = 0.0;

	bool m_SelectionEnable = true;

	std::map<int, std::function<void()>> m_KeyCallbackMap;
};

