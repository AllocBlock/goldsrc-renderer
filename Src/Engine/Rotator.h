#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class CRotator
{
public:
    CRotator() = default;
    CRotator(glm::quat v) { setByQuaternion(v); }
    CRotator(glm::vec3 v) { setByEulerRadians(v); }
    CRotator(glm::mat4 v) { setByRotationMatrix(v); }

    glm::quat getQuaternion() const { return m_Quaternion; }
    glm::vec3 getEulerRadians() const { return glm::eulerAngles(m_Quaternion); }
    glm::vec3 getEulerDegrees() const { return glm::eulerAngles(m_Quaternion) * 180.0f / glm::pi<float>(); }
    glm::mat4 getRotateMatrix() const { return glm::mat4_cast(m_Quaternion); }
    glm::vec3 getOrientation() const { return m_Quaternion * glm::vec3(0.0f, 0.0f, 1.0f); }

    void setByQuaternion(glm::quat v) { m_Quaternion = v; }
    void setByEulerRadians(glm::vec3 v) { m_Quaternion = glm::quat(v); }
    void setEulerDegrees(glm::vec3 v) { m_Quaternion = glm::quat(v * glm::pi<float>() / 180.0f); }
    void setByRotationMatrix(glm::mat4 v) { m_Quaternion = glm::quat_cast(v); }

    CRotator operator * (const CRotator& v2) const
    {
        return CRotator(getQuaternion() * v2.getQuaternion());
    }

    static CRotator createVectorToVector(glm::vec3 vStart, glm::vec3 vTarget);

private:
    glm::quat m_Quaternion = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

