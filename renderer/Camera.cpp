#include "Camera.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

void CCamera::reset()
{
    m_Pos = glm::vec3();
    m_Up = glm::vec3(0.0f, 0.0f, 1.0f);
    m_Phi = 180.0;
    m_Theta = 90.0;
    m_Aspect = 1.0f;
    m_Near = 0.01f;
    m_Far = 2000.0f;
    m_Fov = 120.0f;
}

glm::vec3 CCamera::getFront() const
{
    double X = glm::sin(glm::radians(m_Theta)) * glm::sin(glm::radians(m_Phi));
    double Z = glm::cos(glm::radians(m_Theta));
    double Y = glm::sin(glm::radians(m_Theta)) * glm::cos(glm::radians(m_Phi));
    return glm::vec3(X, Y, Z);
}

glm::mat4 CCamera::getProjMat() const
{
    glm::mat4 Proj = glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
    Proj[1][1] *= -1;
    return Proj;
}

glm::mat4 CCamera::getViewMat() const
{
    glm::vec3 At = m_Pos + getFront();
    return glm::lookAt(m_Pos, At, m_Up);
}