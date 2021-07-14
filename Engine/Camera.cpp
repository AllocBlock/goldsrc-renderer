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
    m_Fov = 90.0f;
}

glm::vec3 CCamera::getFront() const
{
    double X = glm::sin(glm::radians(m_Theta)) * glm::cos(glm::radians(m_Phi));
    double Y = glm::sin(glm::radians(m_Theta)) * glm::sin(glm::radians(m_Phi));
    double Z = glm::cos(glm::radians(m_Theta));
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
    glm::mat4 ViewMat = glm::lookAt(m_Pos, At, m_Up);
    return ViewMat;
}

glm::mat4 CCamera::getViewProjMat() const
{
    return getProjMat() * getViewMat();
}

SFrustum CCamera::getFrustum() const
{
    // get frustum planes for frustum culling
    // more at: https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling
    // x,y,z,w -> A,B,C,D, Ax+By+Cz+D=0, (A,B,C) is the normal of plane

    glm::mat4 VP = getProjMat() * getViewMat();
    SFrustum Frustum;
    // Left Frustum Plane
    // Add first column of the matrix to the fourth column
    Frustum.Planes[0].x = VP[0][3] + VP[0][0];
    Frustum.Planes[0].y = VP[1][3] + VP[1][0];
    Frustum.Planes[0].z = VP[2][3] + VP[2][0];
    Frustum.Planes[0].w = VP[3][3] + VP[3][0];

    // Right Frustum Plane
    // Subtract first column of matrix from the fourth column
    Frustum.Planes[1].x = VP[0][3] - VP[0][0];
    Frustum.Planes[1].y = VP[1][3] - VP[1][0];
    Frustum.Planes[1].z = VP[2][3] - VP[2][0];
    Frustum.Planes[1].w = VP[3][3] - VP[3][0];

    // Top Frustum Plane
    // Subtract second column of matrix from the fourth column
    Frustum.Planes[2].x = VP[0][3] - VP[0][1];
    Frustum.Planes[2].y = VP[1][3] - VP[1][1];
    Frustum.Planes[2].z = VP[2][3] - VP[2][1];
    Frustum.Planes[2].w = VP[3][3] - VP[3][1];

    // Bottom Frustum Plane
    // Add second column of the matrix to the fourth column
    Frustum.Planes[3].x = VP[0][3] + VP[0][1];
    Frustum.Planes[3].y = VP[1][3] + VP[1][1];
    Frustum.Planes[3].z = VP[2][3] + VP[2][1];
    Frustum.Planes[3].w = VP[3][3] + VP[3][1];

    // Near Frustum Plane
    // We could add the third column to the fourth column to get the near plane,
    // but we don't have to do this because the third column IS the near plane
    Frustum.Planes[4].x = VP[0][3];
    Frustum.Planes[4].y = VP[1][3];
    Frustum.Planes[4].z = VP[2][3];
    Frustum.Planes[4].w = VP[3][3];

    // Far Frustum Plane
    // Subtract third column of matrix from the fourth column
    Frustum.Planes[5].x = VP[0][3] - VP[0][2];
    Frustum.Planes[5].y = VP[1][3] - VP[1][2];
    Frustum.Planes[5].z = VP[2][3] - VP[2][2];
    Frustum.Planes[5].w = VP[3][3] - VP[3][2];

    for (int i = 0; i < 6; ++i)
    {
        float Length = static_cast<float>(glm::length(glm::vec3(Frustum.Planes[i].x, Frustum.Planes[i].y, Frustum.Planes[i].z)));
        Frustum.Planes[i] /= Length;
    }
    return Frustum;
}

void CCamera::setAt(glm::vec3 vAt)
{
    const float& Pi = glm::pi<float>();
    float Dx = vAt.x - m_Pos.x;
    float Dy = vAt.y - m_Pos.y;
    float Dz = vAt.z - m_Pos.z;
    float Phi = 0.0f;
    if (Dy == 0.0f)
    {
        if (Dx >= 0)
            Phi = 0.0f;
        else
            Phi = Pi;
    }
    else
    {
        Phi = glm::atan(abs(Dy / Dx));
        if (Dx > 0)
        {
            if (Dy < 0)
                Phi = 2 * Pi - Phi;
        }
        else
        {
            if (Dy > 0)
                Phi = Pi - Phi;
            else
                Phi = Pi + Phi;
        }
    }
    
    m_Phi = Phi / Pi * 180;

    float Length = glm::sqrt(Dx * Dx + Dy * Dy + Dz * Dz);
    if (Length == 0)
        m_Theta = 0;
    else
        m_Theta = glm::acos(Dz / Length) / Pi * 180;
}