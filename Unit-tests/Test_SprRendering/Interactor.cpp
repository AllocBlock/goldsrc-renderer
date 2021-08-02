#include "Interactor.h"
#include "imgui.h"
#include <chrono>
#include <glm/matrix.hpp>

void CInteractor::bindEvent(GLFWwindow* vWindow, std::shared_ptr<CRendererTest> vRenderer)
{
	m_pWindow = vWindow;
	m_pRenderer = vRenderer;

	_ASSERTE(m_pWindow);
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetKeyCallback(m_pWindow, CInteractor::onKeyboard);
	glfwSetCursorPosCallback(m_pWindow, CInteractor::onMouseMove);
}

void CInteractor::onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods)
{
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	if (vAction == GLFW_PRESS &&
		(ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse))
		return;

	switch (vKey)
	{
	case GLFW_KEY_SPACE:
	{
		if (vAction == GLFW_PRESS)
		{
			pInteractor->m_IsMoving = true;
			glfwSetInputMode(vpWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			glfwGetCursorPos(vpWindow, &pInteractor->m_LastMousePosX, &pInteractor->m_LastMousePosY);

			std::shared_ptr<CCamera> pCamera = pInteractor->getRenderer()->getCamera();
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
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	std::shared_ptr<CCamera> pCamera = pInteractor->getRenderer()->getCamera();

	if (!pInteractor->m_IsMoving) return;

	pCamera->setPhi(pInteractor->m_LastPhi + (pInteractor->m_LastMousePosX - vPosX) * pInteractor->m_HorizontalSensetivity);
	pCamera->setTheta(std::min(std::max(pInteractor->m_LastTheta - (pInteractor->m_LastMousePosY - vPosY) * pInteractor->m_VerticalSensetivity, 1.0), 179.0));
}

void CInteractor::update()
{
	float DeltaTime = __getDeltaTime();

	float Scale = 1.0;
	float MoveForward = 0.0, MoveLeft = 0.0;

	int MoveState = __getCurrentMoveState();
	if (MoveState == EMoveState::STOP) return;
	if (MoveState & EMoveState::BOOST) Scale *= m_BoostScale;
	if (MoveState & EMoveState::CRAWL) Scale *= m_CrawlScale;
	if (MoveState & EMoveState::FRONT) MoveForward += 1;
	if (MoveState & EMoveState::BEHIND) MoveForward -= 1;
	if (MoveState & EMoveState::LEFT) MoveLeft += 1;
	if (MoveState & EMoveState::RIGHT) MoveLeft -= 1;

	const auto& pCamera = m_pRenderer->getCamera();
	glm::vec3 Front = pCamera->getFront();
	glm::vec3 Left = pCamera->getLeft();
	glm::vec3 Move = (Front * MoveForward + Left * MoveLeft) * Scale * m_Speed * DeltaTime;
	pCamera->setPos(pCamera->getPos() + Move);
}

void CInteractor::reset()
{
	auto pCamera = m_pRenderer->getCamera();
	pCamera->reset();
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