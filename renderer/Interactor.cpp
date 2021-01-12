#include "Interactor.h"
#include "imgui.h"
#include <chrono>

CInteractor::CInteractor(GLFWwindow* vpWindow, std::shared_ptr<CCamera> vpCamera)
	:m_pWindow(vpWindow),
	m_pCamera(vpCamera)
{
}

void CInteractor::bindEvent()
{
	_ASSERTE(m_pWindow);
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetKeyCallback(m_pWindow, CInteractor::onKeyboard);
	glfwSetCursorPosCallback(m_pWindow, CInteractor::onMouseMove);
	glfwSetMouseButtonCallback(m_pWindow, CInteractor::onMouseClick);
}

void CInteractor::onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods)
{
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse) return;

	switch (vKey)
	{
		case GLFW_KEY_SPACE:
		{
			if (vAction == GLFW_PRESS)
			{
				pInteractor->m_IsMoving = true;
				glfwSetInputMode(vpWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

				glfwGetCursorPos(vpWindow, &pInteractor->m_LastMousePosX, &pInteractor->m_LastMousePosY);

				std::shared_ptr<CCamera> pCamera = pInteractor->getCamera();
				pInteractor->m_LastPhi = pCamera->getPhi();
				pInteractor->m_LastTheta = pCamera->getTheta();
			}
			else if (vAction == GLFW_RELEASE)
			{
				pInteractor->m_IsMoving = false;
				glfwSetInputMode(vpWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			break;
		}
		default:
			break;
	}
}

void CInteractor::onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY)
{
	if (ImGui::GetIO().WantCaptureMouse) return;

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	std::shared_ptr<CCamera> pCamera = pInteractor->getCamera();

	if (!pInteractor->m_IsMoving) return;

	pCamera->setPhi(pInteractor->m_LastPhi - (pInteractor->m_LastMousePosX - vPosX) * pInteractor->m_HorizontalSensetivity);
	pCamera->setTheta(std::min(std::max(pInteractor->m_LastTheta - (pInteractor->m_LastMousePosY - vPosY) * pInteractor->m_VerticalSensetivity, 1.0), 179.0));
}

void CInteractor::onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods)
{
	if (ImGui::GetIO().WantCaptureMouse) return;

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	
	if (!pInteractor->m_Enabled) return;
}

void CInteractor::update()
{
	float DeltaTime = __getDeltaTime();

	float Boost = 1.0;
	float MoveForward = 0.0, MoveLeft = 0.0;

	int MoveState = __getCurrentMoveState();
	if (MoveState == EMoveState::STOP) return;
	if (MoveState & EMoveState::BOOST) Boost *= m_BoostScale;
	if (MoveState & EMoveState::CRAWL) Boost *= m_CrawlScale;
	if (MoveState & EMoveState::FRONT) MoveForward += 1;
	if (MoveState & EMoveState::BEHIND) MoveForward -= 1;
	if (MoveState & EMoveState::LEFT) MoveLeft += 1;
	if (MoveState & EMoveState::RIGHT) MoveLeft -= 1;

	glm::vec3 Front = m_pCamera->getFront();
	glm::vec3 Left = m_pCamera->getLeft();
	glm::vec3 Move = (Front * MoveForward + Left * MoveLeft) * Boost * m_Speed * DeltaTime;
	m_pCamera->setPos(m_pCamera->getPos() + Move);
}

void CInteractor::reset()
{
	m_pCamera->reset();
	m_Speed = 3.0f;
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
	if (KeyW) MoveState |= EMoveState::FRONT;

	bool KeyS = glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS;
	if (KeyS) MoveState |= EMoveState::BEHIND;

	bool KeyA = glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS;
	if (KeyA) MoveState |= EMoveState::LEFT;

	bool KeyD = glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS;
	if (KeyD) MoveState |= EMoveState::RIGHT;

	bool KeyLeftShift = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
	if (KeyLeftShift) MoveState |= EMoveState::BOOST;

	bool KeyLeftCtrl = glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
	if (KeyLeftCtrl) MoveState |= EMoveState::CRAWL;
	
	return MoveState;
}