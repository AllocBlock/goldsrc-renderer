#include "Camera.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

glm::vec3 CCamera::getFront() const
{
    double X = glm::sin(glm::radians(m_Theta)) * glm::sin(glm::radians(m_Phi));
    double Z = glm::cos(glm::radians(m_Theta));
    double Y = glm::sin(glm::radians(m_Theta)) * glm::cos(glm::radians(m_Phi));
    return glm::vec3(X, Y, Z);
}

glm::mat4 CCamera::getProjMat() const
{
    glm::mat4 Proj = glm::perspective(m_Fov, m_Aspect, m_Near, m_Far);
    Proj[1][1] *= -1;
    return Proj;
}

glm::mat4 CCamera::getViewMat() const
{
    glm::vec3 At = m_Pos + getFront();
    return glm::lookAt(m_Pos, At, m_Up);
}