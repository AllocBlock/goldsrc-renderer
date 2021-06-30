#include "Interactor.h"
#include "imgui.h"
#include <chrono>
#include <glm/matrix.hpp>

void CInteractor::bindEvent(GLFWwindow* vWindow, std::shared_ptr<CVulkanRenderer> vRenderer)
{
	m_pWindow = vWindow;
	m_pRenderer = vRenderer;

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

void CInteractor::onMouseClick(GLFWwindow* vpWindow, int vButton, int vAction, int vMods)
{
	if (ImGui::GetIO().WantCaptureMouse) return;

	CInteractor* pInteractor = reinterpret_cast<CInteractor*>(glfwGetWindowUserPointer(vpWindow));
	
	if (!pInteractor->m_Enabled) return;

	if (vButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (pInteractor->m_SelectionEnable)
		{
			double XPos = 0.0, YPos = 0.0;
			glfwGetCursorPos(vpWindow, &XPos, &YPos);
			pInteractor->__selectByClick(glm::vec2(XPos, YPos));
		}
	}
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

void CInteractor::__selectByClick(glm::vec2 vPos)
{
	const auto& pScene = m_pRenderer->getScene();
	const auto& pCamera = m_pRenderer->getCamera();

	if (!pScene || pScene->Objects.empty()) return;
	// normolize screen coordinate
	int WindowWidth = 0, WindowHeight = 0;
	glfwGetFramebufferSize(m_pWindow, &WindowWidth, &WindowHeight);
	glm::vec4 ScreenPos = glm::vec4(vPos.x / WindowWidth * 2 - 1.0, vPos.y / WindowHeight * 2 - 1.0, 1.0f, 1.0f);

	// screen coordinate to world coordinate
	// use inverse matrix mult
	glm::mat4 ProjMat = pCamera->getProjMat();
	glm::mat4 ViewMat = pCamera->getViewMat();

	glm::mat4 InversedVP = glm::inverse(ProjMat * ViewMat);
	glm::vec4 WorldPosHC = InversedVP * ScreenPos;
	WorldPosHC /= WorldPosHC.w;
	glm::vec3 WorldPos = glm::vec3(WorldPosHC);

	// construct the ray
	glm::vec3 EyePos = pCamera->getPos();
	glm::vec3 Direction = glm::normalize(WorldPos - EyePos);

	float NearestDistance = INFINITY;
	std::optional<S3DBoundingBox> NearestBoundingBox = std::nullopt;

	int temp = 0;
	for (const auto& pObject : pScene->Objects)
	{
		temp++;
		std::optional<S3DBoundingBox> BB = pObject->getBoundingBox();
		if (BB == std::nullopt) continue;
		// TODO: get intersection of ray and bounding box
		// get distance, compare with currert neareset
		// save nearest bounding box
		float NearT = 0.0f, FarT = 0.0f;
		bool HasIntersection = __getIntersectionOfRayAndBoundingBox(EyePos, Direction, BB.value(), NearT, FarT);
		if (HasIntersection && NearT < NearestDistance)
		{
			NearestDistance = NearT;
			NearestBoundingBox = BB;
		}
	}

	if (NearestBoundingBox.has_value())
	{
		m_pRenderer->setHighlightBoundingBox(NearestBoundingBox.value());
		m_pRenderer->addGuiLine("EyeRay", EyePos, EyePos + NearestDistance * Direction);
	}
	else
		m_pRenderer->removeHighlightBoundingBox(); 
}

bool CInteractor::__getIntersectionOfRayAndBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, S3DBoundingBox vBB, float& voNearT, float& voFarT)
{
	vDirection = glm::normalize(vDirection);


	size_t NumZero = 0;
	if (vDirection.x == 0.0) NumZero++;
	if (vDirection.y == 0.0) NumZero++;
	if (vDirection.z == 0.0) NumZero++;

	if (NumZero == 3)
	{
		throw std::runtime_error(u8"求交的射线方向是零向量");
	}
	else if (NumZero == 2) // 1D
	{
		// one main axis and two zero axes, for reusing 3 conditions on x, y and z axes
		uint8_t AxisMap[3] = { 0, 1, 2 };

		// find main axis and update AxisMap
		for (uint8_t i = 0; i < 3; ++i)
		{
			if (vDirection[i] != 0)
			{
				AxisMap[0] = i;
				AxisMap[i] = 0;
			}
		}

		uint8_t MainAxis = AxisMap[0], ZeroAxis1 = AxisMap[1], ZeroAxis2 = AxisMap[2];

		if (vOrigin[ZeroAxis1] < vBB.Min[ZeroAxis1] || vOrigin[ZeroAxis1] > vBB.Max[ZeroAxis1] || // outside on zero axis 1
			vOrigin[ZeroAxis2] < vBB.Min[ZeroAxis2] || vOrigin[ZeroAxis2] > vBB.Max[ZeroAxis2] || // outside on zero axis 2
			vDirection[MainAxis] > 0 ? vOrigin[MainAxis] > vBB.Max[MainAxis] : vOrigin[MainAxis] < vBB.Min[MainAxis])// outside on main axis 
		{
			return false;
		}
		float T1 = (vBB.Min[MainAxis] - vOrigin[MainAxis]) / vDirection[MainAxis];
		float T2 = (vBB.Max[MainAxis] - vOrigin[MainAxis]) / vDirection[MainAxis];
		voNearT = std::min<float>(T1, T2);
		voFarT = std::max<float>(T1, T2);
		if (voNearT < 0) voNearT = voFarT;
		return true;
	}
	else if (NumZero == 1) // 2D
	{
		// two main axes and two zero axis
		uint8_t AxisMap[3] = { 0, 1, 2 };

		// find main axis and update AxisMap
		for (uint8_t i = 0; i < 3; ++i)
		{
			if (vDirection[i] == 0)
			{
				AxisMap[2] = i;
				AxisMap[i] = 2;
			}
		}

		uint8_t MainAxis1 = AxisMap[0], MainAxis2 = AxisMap[1], ZeroAxis = AxisMap[2];

		if (vOrigin[ZeroAxis] < vBB.Min[ZeroAxis] || vOrigin[ZeroAxis] > vBB.Max[ZeroAxis] || // outside on zero axis
			vDirection[MainAxis1] > 0 ? vOrigin[MainAxis1] > vBB.Max[MainAxis1] : vOrigin[MainAxis1] < vBB.Min[MainAxis1] ||
			vDirection[MainAxis2] > 0 ? vOrigin[MainAxis2] > vBB.Max[MainAxis2] : vOrigin[MainAxis2] < vBB.Min[MainAxis2])// outside on main axis
		{
			return false;
		}
		float Axis1T1 = (vBB.Min[MainAxis1] - vOrigin[MainAxis1]) / vDirection[MainAxis1];
		float Axis1T2 = (vBB.Max[MainAxis1] - vOrigin[MainAxis1]) / vDirection[MainAxis1];
		float Axis2T1 = (vBB.Min[MainAxis2] - vOrigin[MainAxis2]) / vDirection[MainAxis2];
		float Axis2T2 = (vBB.Max[MainAxis2] - vOrigin[MainAxis2]) / vDirection[MainAxis2];
		float Axis1NearT = std::min<float>(Axis1T1, Axis1T2);
		float Axis1FarT = std::max<float>(Axis1T1, Axis1T2);
		float Axis2NearT = std::min<float>(Axis2T1, Axis2T2);
		float Axis2FarT = std::max<float>(Axis2T1, Axis2T2);

		float NearT = std::max<float>(Axis1NearT, Axis2NearT);
		float FarT = std::min<float>(Axis1FarT, Axis2FarT);
		if (NearT > FarT)
			return false;
		else
		{
			voNearT = NearT;
			voFarT = FarT;
			if (voNearT < 0) voNearT = voFarT;
			return true;
		}
	}
	else // 3D
	{
		if (vDirection.x > 0 ? vOrigin.x > vBB.Max.x : vOrigin.x < vBB.Min.x ||
			vDirection.y > 0 ? vOrigin.y > vBB.Max.y : vOrigin.y < vBB.Min.y ||
			vDirection.z > 0 ? vOrigin.z > vBB.Max.z : vOrigin.z < vBB.Min.z)
			// outside on any axis
		{
			return false;
		}
		float Axis1T1 = (vBB.Min.x - vOrigin.x) / vDirection.x;
		float Axis1T2 = (vBB.Max.x - vOrigin.x) / vDirection.x;
		float Axis2T1 = (vBB.Min.y - vOrigin.y) / vDirection.y;
		float Axis2T2 = (vBB.Max.y - vOrigin.y) / vDirection.y;
		float Axis3T1 = (vBB.Min.z - vOrigin.z) / vDirection.z;
		float Axis3T2 = (vBB.Max.z - vOrigin.z) / vDirection.z;
		float Axis1NearT = std::min<float>(Axis1T1, Axis1T2);
		float Axis1FarT = std::max<float>(Axis1T1, Axis1T2);
		float Axis2NearT = std::min<float>(Axis2T1, Axis2T2);
		float Axis2FarT = std::max<float>(Axis2T1, Axis2T2);
		float Axis3NearT = std::min<float>(Axis3T1, Axis3T2);
		float Axis3FarT = std::max<float>(Axis3T1, Axis3T2);

		float NearT = std::max<float>(Axis1NearT, std::max<float>(Axis2NearT, Axis3NearT));
		float FarT = std::min<float>(Axis1FarT, std::min<float>(Axis2FarT, Axis3FarT));
		if (NearT > FarT)
			return false;
		else
		{
			voNearT = NearT;
			voFarT = FarT;
			if (voNearT < 0) voNearT = voFarT;
			return true;
		}
	}
}