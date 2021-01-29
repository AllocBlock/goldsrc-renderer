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

bool CCamera::isObjectInSight(std::shared_ptr<S3DObject> vpObject) const
{
    // AABB frustum culling
    std::array<glm::vec4, 6> FrustumPlanes = __getFrustumPlanes();
    S3DBoundingBox BoundingBox = vpObject->getBoundingBox();
    std::array<glm::vec3, 8> BoundPoints;
    for (int i = 0; i < 8; ++i)
    {
        int SignX = ((i & 1) ? 1 : -1);
        int SignY = ((i & 2) ? 1 : -1);
        int SignZ = ((i & 4) ? 1 : -1);
        float X = ((i & 1) ? BoundingBox.Min.x : BoundingBox.Max.x);
        float Y = ((i & 2) ? BoundingBox.Min.y : BoundingBox.Max.y);
        float Z = ((i & 4) ? BoundingBox.Min.z : BoundingBox.Max.z);
        BoundPoints[i] = glm::vec3(X, Y, Z);
    }

    // for each frustum plane
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 Normal = glm::vec3(FrustumPlanes[i].x, FrustumPlanes[i].y, FrustumPlanes[i].z);
        float D = FrustumPlanes[i].w;
        // if all of the vertices in bounding is behind this plane, the object should not be drawn
        bool NoDraw = true;
        for (int k = 0; k < 8; ++k)
        {
            if (glm::dot(Normal, BoundPoints[k]) + D > 0)
            {
                NoDraw = false;
                break;
            }
        }
        if (NoDraw) return false;
    }
    return true;
}

std::array<glm::vec4, 6> CCamera::__getFrustumPlanes() const
{
    // get frustum planes for frustum culling
    // more at: https://www.braynzarsoft.net/viewtutorial/q16390-34-aabb-cpu-side-frustum-culling
    // x,y,z,w -> A,B,C,D, Ax+By+Cz+D=0, (A,B,C) is the normal of plane

    glm::mat4 VP = getProjMat() * getViewMat();
    std::array<glm::vec4, 6> FrustumPlanes;
    // Left Frustum Plane
    // Add first column of the matrix to the fourth column
    FrustumPlanes[0].x = VP[0][3] + VP[0][0];
    FrustumPlanes[0].y = VP[1][3] + VP[1][0];
    FrustumPlanes[0].z = VP[2][3] + VP[2][0];
    FrustumPlanes[0].w = VP[3][3] + VP[3][0];

    // Right Frustum Plane
    // Subtract first column of matrix from the fourth column
    FrustumPlanes[1].x = VP[0][3] - VP[0][0];
    FrustumPlanes[1].y = VP[1][3] - VP[1][0];
    FrustumPlanes[1].z = VP[2][3] - VP[2][0];
    FrustumPlanes[1].w = VP[3][3] - VP[3][0];

    // Top Frustum Plane
    // Subtract second column of matrix from the fourth column
    FrustumPlanes[2].x = VP[0][3] - VP[0][1];
    FrustumPlanes[2].y = VP[1][3] - VP[1][1];
    FrustumPlanes[2].z = VP[2][3] - VP[2][1];
    FrustumPlanes[2].w = VP[3][3] - VP[3][1];

    // Bottom Frustum Plane
    // Add second column of the matrix to the fourth column
    FrustumPlanes[3].x = VP[0][3] + VP[0][1];
    FrustumPlanes[3].y = VP[1][3] + VP[1][1];
    FrustumPlanes[3].z = VP[2][3] + VP[2][1];
    FrustumPlanes[3].w = VP[3][3] + VP[3][1];

    // Near Frustum Plane
    // We could add the third column to the fourth column to get the near plane,
    // but we don't have to do this because the third column IS the near plane
    FrustumPlanes[4].x = VP[0][3];
    FrustumPlanes[4].y = VP[1][3];
    FrustumPlanes[4].z = VP[2][3];
    FrustumPlanes[4].w = VP[3][3];

    // Far Frustum Plane
    // Subtract third column of matrix from the fourth column
    FrustumPlanes[5].x = VP[0][3] - VP[0][2];
    FrustumPlanes[5].y = VP[1][3] - VP[1][2];
    FrustumPlanes[5].z = VP[2][3] - VP[2][2];
    FrustumPlanes[5].w = VP[3][3] - VP[3][2];

    for (int i = 0; i < 6; ++i)
    {
        float Length = glm::vec3(FrustumPlanes[i].x, FrustumPlanes[i].y, FrustumPlanes[i].z).length();
        FrustumPlanes[i] /= Length;
    }
    return FrustumPlanes;
}