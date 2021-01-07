#include "Interactor.h"
#include "imgui.h"

CInteractor::CInteractor(CVulkanRenderer* vpRender)
	:m_pRenderer(vpRender)
{
}

void CInteractor::bindEvent()
{
	_ASSERTE(m_pRenderer);
	GLFWwindow* pWindow = m_pRenderer->getWindow();
	_ASSERTE(pWindow);

	glfwSetWindowUserPointer(pWindow, this);
	glfwSetKeyCallback(pWindow, CInteractor::onKeyboard);
	glfwSetCursorPosCallback(pWindow, CInteractor::onMouseMove);
	glfwSetMouseButtonCallback(pWindow, CInteractor::onMouseClick);
}

void CInteractor::onKeyboard(GLFWwindow* vpWindow, int vKey, int vScancode, int vAction, int vMods)
{
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	if (ImGui::GetIO().WantCaptureKeyboard) return;

	int& MoveState = pInteractor->m_MoveState;
	if (vKey == GLFW_KEY_W)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::FRONT;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::FRONT;
	}
	else if (vKey == GLFW_KEY_S)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::BEHIND;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::BEHIND;
	}
	else if (vKey == GLFW_KEY_A)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::LEFT;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::LEFT;
	}
	else  if (vKey == GLFW_KEY_D)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::RIGHT;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::RIGHT;
	}
	else  if (vKey == GLFW_KEY_LEFT_SHIFT)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::BOOST;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::BOOST;
	}
	else  if (vKey == GLFW_KEY_LEFT_CONTROL)
	{
		if (vAction == GLFW_PRESS) MoveState |= EMoveState::SLOW;
		else if (vAction == GLFW_RELEASE) MoveState &= ~EMoveState::SLOW;
	}
}

void CInteractor::onMouseMove(GLFWwindow* vpWindow, double vPosX, double vPosY)
{
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	CCamera* pCamera = pInteractor->m_pRenderer->getCamera();

	if (!pInteractor->m_IsMoving) return;

	pCamera->setPhi(pInteractor->m_LastPhi - (pInteractor->m_LastMousePosX - vPosX) * pInteractor->m_HorizontalSensetivity);
	pCamera->setTheta(std::min(std::max(pInteractor->m_LastTheta - (pInteractor->m_LastMousePosY - vPosY) * pInteractor->m_VerticalSensetivity, 1.0), 179.0));
}

void CInteractor::onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods)
{
	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	if (!pInteractor->m_Enabled) return;
	const CCamera* pCamera = pInteractor->m_pRenderer->getCamera();

	if (vButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (vAction == GLFW_PRESS)
		{
			if (ImGui::GetIO().WantCaptureMouse) return;
			pInteractor->m_IsMoving = true;
			glfwGetCursorPos(vpWindow, &pInteractor->m_LastMousePosX, &pInteractor->m_LastMousePosY);
			
			pInteractor->m_LastPhi = pCamera->getPhi();
			pInteractor->m_LastTheta = pCamera->getTheta();
		}
		else if (vAction == GLFW_RELEASE)
		{
			pInteractor->m_IsMoving = false;
		}
	}
}

void CInteractor::update(float vDeltaTime)
{
	float Boost = 1.0;
	float MoveForward = 0.0, MoveLeft = 0.0;
	if (m_MoveState == EMoveState::STOP) return;
	if (m_MoveState & EMoveState::BOOST) Boost *= m_BoostScale;
	if (m_MoveState & EMoveState::SLOW) Boost *= m_CrawlScale;
	if (m_MoveState & EMoveState::FRONT) MoveForward += 1;
	if (m_MoveState & EMoveState::BEHIND) MoveForward -= 1;
	if (m_MoveState & EMoveState::LEFT) MoveLeft += 1;
	if (m_MoveState & EMoveState::RIGHT) MoveLeft -= 1;

	CCamera* pCamera = m_pRenderer->getCamera();
	glm::vec3 Front = pCamera->getFront();
	glm::vec3 Left = pCamera->getLeft();
	glm::vec3 Move = (Front * MoveForward + Left * MoveLeft) * Boost * m_Speed * vDeltaTime;
	pCamera->setPos(pCamera->getPos() + Move);
}

