#pragma once

#include <glm/glm.hpp>

class CCamera
{
public:
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

private:
	glm::vec3 m_Pos = glm::vec3();
	glm::vec3 m_Up = glm::vec3(0.0f, 0.0f, 1.0f);
	double m_Phi = 180.0;
	double m_Theta = 90.0;
	float m_Fov = 120.0f;
	float m_Aspect = 1.0f;
	float m_Near = 0.01f;
	float m_Far = 2000.0f;
};

