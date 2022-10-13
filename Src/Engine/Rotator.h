#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class CRotator
{
public:
    CRotator() {}
    CRotator(glm::quat v) { setByQuaternion(v); }
    CRotator(glm::vec3 v) { setByEulerRadians(v); }
    CRotator(glm::mat4 v) { setByRotationMatrix(v); }

    glm::quat getQuaternion() const { return m_Rotation; }
    glm::vec3 getEulerRadians() const { return glm::eulerAngles(m_Rotation); }
    glm::vec3 getEulerDegrees() const { return glm::eulerAngles(m_Rotation) * 180.0f / glm::pi<float>(); }
    glm::mat4 getRotateMatrix() const { return glm::mat4_cast(m_Rotation); }

    void setByQuaternion(glm::quat v) { m_Rotation = v; }
    void setByEulerRadians(glm::vec3 v) { m_Rotation = glm::quat(v); }
    void setEulerDegrees(glm::vec3 v) { m_Rotation = glm::quat(v * glm::pi<float>() / 180.0f); }
    void setByRotationMatrix(glm::mat4 v) { m_Rotation = glm::quat_cast(v); }

private:
    glm::quat m_Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

