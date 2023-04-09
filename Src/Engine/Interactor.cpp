#include "Interactor.h"
#include "InterfaceUI.h"
#include <chrono>

enum class EMoveState
{
	STOP = 0x0000,
	FRONT = 0x0001,
	BEHIND = 0x0002,
	LEFT = 0x0004,
	RIGHT = 0x0008,
	UP = 0x0010,
	DOWN = 0x0020,
	BOOST = 0x0040,
	CRAWL = 0x0080,
};

enum class ERotateState
{
	STOP = 0x0000,
	LEFT = 0x0001,
	RIGHT = 0x0002,
	UP = 0x0004,
	DOWN = 0x0008,
	BOOST = 0x0010,
	CRAWL = 0x0020,
};

CInteractor::CInteractor()
{
	m_Timer.start();
}

void CInteractor::bindEvent(GLFWwindow* vWindow, CCamera::Ptr vCamera)
{
	m_pWindow = vWindow;
	m_pCamera = vCamera;

	_ASSERTE(m_pWindow);
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetKeyCallback(m_pWindow, CInteractor::onKeyboard);
	glfwSetCursorPosCallback(m_pWindow, CInteractor::onMouseMove);
	glfwSetMouseButtonCallback(m_pWindow, CInteractor::onMouseClick);
}

void CInteractor::onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods)
{
	if (UI::isUsingKeyboard()) return;

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;

	if (vAction == GLFW_PRESS || vAction == GLFW_REPEAT)
	{
		if (pInteractor->m_KeyCallbackMap.find(vKey) != pInteractor->m_KeyCallbackMap.end())
		{
			pInteractor->m_KeyCallbackMap[vKey]();
		}
		else
		{
			// 默认行为
			if (vKey == GLFW_KEY_CAPS_LOCK)
			{
				if (pInteractor->m_IsMoving)
					pInteractor->__stopMovingMode();
				else
					pInteractor->__startMovingMode();
			}
			else if (vKey == GLFW_KEY_EQUAL)
				pInteractor->m_Speed *= 1.05f;
			else if (vKey == GLFW_KEY_MINUS)
				pInteractor->m_Speed *= 0.95f;
			else if (vKey == GLFW_KEY_ESCAPE)
				pInteractor->__stopMovingMode();
		}
	}
}

void CInteractor::onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY)
{
    if (UI::isInited() && UI::isUsingMouse()) return;

	float PosX = static_cast<float>(vPosX);
	float PosY = static_cast<float>(vPosY);

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled || !pInteractor->m_IsMoving) return;

	CCamera::Ptr pCamera = pInteractor->m_pCamera;
	pCamera->setPhi(pInteractor->m_LastPhi - (pInteractor->m_LastMousePosX - PosX) * pInteractor->m_HorizontalSensetivity);
	pCamera->setTheta(glm::clamp(pInteractor->m_LastTheta - (pInteractor->m_LastMousePosY - PosY) * pInteractor->m_VerticalSensetivity, 1.0f, 179.0f));
}

void CInteractor::onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods)
{
    if (UI::isUsingMouse()) return;

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	
	if (!pInteractor->m_Enabled) return;

    if (pInteractor->m_pMouseCallback)
		pInteractor->m_pMouseCallback(vpWindow, vButton, vAction);

	if (vButton == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (vAction == GLFW_PRESS)
			pInteractor->__startMovingMode();
		else if (vAction == GLFW_RELEASE)
			pInteractor->__stopMovingMode();
	}
}

void CInteractor::update()
{
	float DeltaTime = m_Timer.tick();
	__updateMove(DeltaTime);
	__updateRotate(DeltaTime);
}

void CInteractor::reset()
{
	m_pCamera->reset();
	m_Speed = 3.0f;
}

void CInteractor::_renderUIV()
{
	if (UI::collapse(u8"Interactor"))
    {    
		// 帮助
        UI::text(u8"操作方法");
		UI::bulletText(u8"按住空格开始控制相机，松开即结束");
		UI::bulletText(u8"WASD前后左右移动");
		UI::bulletText(u8"按住左Shift键加速，按住左Ctrl键减速");
		UI::bulletText(u8"移动鼠标转动视角");
		UI::split();

		UI::slider(u8"速度", m_Speed, 100.0f, 2000.0f, "%.1f");
	}
}

float CInteractor::__getDeltaTime()
{
	static std::chrono::milliseconds LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	std::chrono::milliseconds CurrentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	float DeltaTime = static_cast<float>((CurrentTimeStamp - LastTimeStamp).count()) / 1000.0f;
	LastTimeStamp = CurrentTimeStamp;
	return DeltaTime;
}

int CInteractor::__getCurrentMoveState()
{
	int MoveState = static_cast<int>(EMoveState::STOP);
	if (!m_IsMoving)
		return MoveState;

	bool KeyW = glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS;
	if (KeyW) MoveState |= int(EMoveState::FRONT);

	bool KeyS = glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS;
	if (KeyS) MoveState |= int(EMoveState::BEHIND);

	bool KeyA = glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS;
	if (KeyA) MoveState |= int(EMoveState::LEFT);

	bool KeyD = glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS;
	if (KeyD) MoveState |= int(EMoveState::RIGHT);

	bool KeyE = glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS;
	if (KeyE) MoveState |= int(EMoveState::UP);

	bool KeyQ = glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS;
	if (KeyQ) MoveState |= int(EMoveState::DOWN);

	bool KeyLeftShift = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
	if (KeyLeftShift) MoveState |= int(EMoveState::BOOST);

	bool KeyLeftCtrl = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	if (KeyLeftCtrl) MoveState |= int(EMoveState::CRAWL);

	return MoveState;
}

int CInteractor::__getCurrentRotateState()
{
	int RotateState = static_cast<int>(ERotateState::STOP);
	if (!m_IsMoving)
		return RotateState;

	bool KeyArrowUp = glfwGetKey(m_pWindow, GLFW_KEY_UP) == GLFW_PRESS;
	if (KeyArrowUp) RotateState |= int(ERotateState::UP);

	bool KeyArrowDown = glfwGetKey(m_pWindow, GLFW_KEY_DOWN) == GLFW_PRESS;
	if (KeyArrowDown) RotateState |= int(ERotateState::DOWN);

	bool KeyArrowLeft = glfwGetKey(m_pWindow, GLFW_KEY_LEFT) == GLFW_PRESS;
	if (KeyArrowLeft) RotateState |= int(ERotateState::LEFT);

	bool KeyArrowRight = glfwGetKey(m_pWindow, GLFW_KEY_RIGHT) == GLFW_PRESS;
	if (KeyArrowRight) RotateState |= int(ERotateState::RIGHT);

	bool KeyLeftShift = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
	if (KeyLeftShift) RotateState |= int(ERotateState::BOOST);

	bool KeyLeftCtrl = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	if (KeyLeftCtrl) RotateState |= int(ERotateState::CRAWL);

	return RotateState;
}

void CInteractor::__startMovingMode()
{
	if (!m_pCamera) return;

	m_IsMoving = true;
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	double LastMousePosX, LastMousePosY;
	glfwGetCursorPos(m_pWindow, &LastMousePosX, &LastMousePosY);
	m_LastMousePosX = static_cast<float>(LastMousePosX);
	m_LastMousePosY = static_cast<float>(LastMousePosY);

	m_LastPhi = m_pCamera->getPhi();
	m_LastTheta = m_pCamera->getTheta();
}

void CInteractor::__stopMovingMode()
{
	m_IsMoving = false;
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void CInteractor::__updateMove(float vDeltaTime)
{
	int MoveState = __getCurrentMoveState();
	if (MoveState == int(EMoveState::STOP)) return;

	float Scale = 1.0;
	float MoveForward = 0.0, MoveLeft = 0.0, MoveUp = 0.0;

	if (MoveState & int(EMoveState::BOOST)) Scale *= m_BoostScale;
	if (MoveState & int(EMoveState::CRAWL)) Scale *= m_CrawlScale;
	if (MoveState & int(EMoveState::FRONT)) MoveForward += 1;
	if (MoveState & int(EMoveState::BEHIND)) MoveForward -= 1;
	if (MoveState & int(EMoveState::LEFT)) MoveLeft += 1;
	if (MoveState & int(EMoveState::RIGHT)) MoveLeft -= 1;
	if (MoveState & int(EMoveState::UP)) MoveUp += 1;
	if (MoveState & int(EMoveState::DOWN)) MoveUp -= 1;

	glm::vec3 Front = m_pCamera->getFront();
	glm::vec3 Left = m_pCamera->getLeft();
	glm::vec3 Up = m_pCamera->getUp();
	glm::vec3 Move = (Front * MoveForward + Left * MoveLeft + Up * MoveUp) * Scale * m_Speed * vDeltaTime;
	m_pCamera->setPos(m_pCamera->getPos() + Move);
}

void CInteractor::__updateRotate(float vDeltaTime)
{
	// 视角
	float Scale = m_Speed * vDeltaTime * 1000.0f;
	int RotateState = __getCurrentRotateState();
	if (RotateState == int(ERotateState::STOP)) return;

	float RotateLeft = 0.0f, RotateUp = 0.0f;
	if (RotateState & int(ERotateState::UP)) RotateUp += 1;
	if (RotateState & int(ERotateState::DOWN)) RotateUp -= 1;
	if (RotateState & int(ERotateState::LEFT)) RotateLeft += 1;
	if (RotateState & int(ERotateState::RIGHT)) RotateLeft -= 1;
	if (RotateState & int(ERotateState::BOOST)) Scale *= m_BoostScale;
	if (RotateState & int(ERotateState::CRAWL)) Scale *= m_CrawlScale;

	m_pCamera->setPhi(m_pCamera->getPhi() + m_HorizontalSensetivity * RotateLeft * Scale);
	m_pCamera->setTheta(glm::clamp(m_pCamera->getTheta() - m_VerticalSensetivity * RotateUp * Scale, 1.0f, 179.0f));
}