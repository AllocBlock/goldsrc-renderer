#pragma once
#include "Scene.h"

#include <glm/glm.hpp>
#include <array>

class CCamera
{
public:
	CCamera() { reset(); }

	void reset();

	glm::mat4 getProjMat() const;
	glm::mat4 getViewMat() const;

	glm::vec3 getPos() const { return m_Pos; };
	glm::vec3 getUp() const { return m_Up; };
	glm::vec3 getFront() const;
	glm::vec3 getLeft() const { return glm::normalize(glm::cross(m_Up, getFront())); };
	double getPhi() const { return m_Phi; };
	double getTheta() const { return m_Theta; };
	float getFov() const { return m_Fov; };
	float getAspect() const { return m_Aspect; };
	float getNear() const { return m_Near; };
	float getFar() const { return m_Far; };

	void setPos(glm::vec3 vPos) { m_Pos = vPos; };
	void setUo(glm::vec3 vUp) { m_Up = vUp; };
	void setPhi(double vPhi) { m_Phi = vPhi; };
	void setTheta(double vTheta) { m_Theta = vTheta; };
	void setFov(float vFov) { m_Fov = vFov; };
	void setAspect(float vAspect) { m_Aspect = vAspect; };
	void setNear(float vNear) { m_Near = vNear; };
	void setFar(float vFar) { m_Far = vFar; };

	bool isObjectInSight(std::shared_ptr<S3DObject> vpObject) const;
private:
	std::array<glm::vec4, 6> __getFrustumPlanes() const;

	glm::vec3 m_Pos;
	glm::vec3 m_Up;
	double m_Phi;
	double m_Theta;
	float m_Aspect;
	float m_Near;
	float m_Far;
	float m_Fov;
};

