#pragma once
#include "Pointer.h"
#include "Common.h"
#include "DrawableUI.h"

#include <glm/glm.hpp>
#include <array>

struct SFrustum
{
	std::array<glm::vec4, 6> Planes = { glm::vec4(), glm::vec4(), glm::vec4(), glm::vec4() };
};

class CCamera : public IDrawableUI
{
public:
	_DEFINE_PTR(CCamera);

	CCamera() { reset(); }

	void reset();

	glm::mat4 getProjMat() const;
	glm::mat4 getViewMat() const;
	glm::mat4 getViewProjMat() const;

	glm::vec3 getPos() const { return m_Pos; }
	glm::vec3 getUp() const { return m_Up; }
	glm::vec3 getFront() const;
	glm::vec3 getLeft() const { return glm::normalize(glm::cross(m_Up, getFront())); }
	float getPhi() const { return m_Phi; }
	float getTheta() const { return m_Theta; }
	float getFov() const { return m_Fov; }
	float getAspect() const { return m_Aspect; }
	float getNear() const { return m_Near; }
	float getFar() const { return m_Far; }

	void setPos(glm::vec3 vPos) { m_Pos = vPos; }
	void setUp(glm::vec3 vUp) { m_Up = vUp; }
	void setPhi(float vPhi) { m_Phi = Common::mod(vPhi, 360.0f);}
	void setTheta(float vTheta) { m_Theta = Common::mod(vTheta, 180.0f); }
	void setFov(float vFov) { m_Fov = vFov; }
	void setAspect(float vAspect) { m_Aspect = vAspect; }
	void setNear(float vNear) { m_Near = vNear; }
	void setFar(float vFar) { m_Far = vFar; }
	void setAt(glm::vec3 vAt);

	SFrustum getFrustum() const;

protected:
	virtual void _renderUIV() override;

private:
	glm::vec3 m_Pos;
	glm::vec3 m_Up;
	float m_Phi;
	float m_Theta;
	float m_Aspect;
	float m_Near;
	float m_Far;
	float m_Fov;
};

