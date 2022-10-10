#pragma once
#include "Pointer.h"
#include "PhysicsState.h"

#include <vector>
#include <glm/glm.hpp>

class CPhysicsEngine
{
public:
    _DEFINE_PTR(CPhysicsEngine);

    _DEFINE_GETTER_SETTER(GravityAcceleration, float)
    _DEFINE_GETTER_SETTER(SimulateSpeed, float)

    void resume() { m_Paused = false; }
    void pause() { m_Paused = true; }
    bool isPaused() const { return m_Paused; }

    void add(ptr<SPhysicsStateRigidBody> vRigid)
    {
        _ASSERTE(vRigid);
        m_RigidBodySet.emplace_back(vRigid);
    }
    void clear() { m_RigidBodySet.clear(); }
    void update(float vDeltaTime) const;

private:
    std::vector<ptr<SPhysicsStateRigidBody>> m_RigidBodySet;

    bool m_Paused = false;
    float m_SimulateSpeed = 1.0f; // time speed
    float m_GravityAcceleration = 9.8f;
};
