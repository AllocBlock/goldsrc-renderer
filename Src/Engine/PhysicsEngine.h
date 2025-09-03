#pragma once
#include "Pointer.h"
#include "PhysicsState.h"

#include <vector>
#include <functional>

class CPhysicsEngine
{
public:
    
    _DEFINE_GETTER_SETTER(GravityAcceleration, float)
    _DEFINE_GETTER_SETTER(SimulateSpeed, float)

    using CollideCallback_t = std::function<void(glm::vec3, glm::vec3)>;

    void resume() { m_Paused = false; }
    void pause() { m_Paused = true; }
    bool isPaused() const { return m_Paused; }

    void addRigidBody(sptr<SPhysicsStateRigidBody> vRigid)
    {
        _ASSERTE(vRigid);
        m_RigidBodySet.emplace_back(vRigid);
    }
    size_t getRigidBodyNum() const { return m_RigidBodySet.size(); }
    sptr<SPhysicsStateRigidBody> getRigidBody(size_t vIndex) const
    {
        _ASSERTE(vIndex < m_RigidBodySet.size());
        return m_RigidBodySet[vIndex];
    }
    void clearRigidBody() { m_RigidBodySet.clear(); }
    void update(float vDeltaTime) const;

    bool isGravityEnabled() const { return m_EnableGravity; }
    void setGravityState(bool vEnable) { m_EnableGravity = vEnable; }

    void addCollisionHook(CollideCallback_t vCallback) { m_CollisionCallbackSet.emplace_back(vCallback); }

private:
    std::vector<sptr<SPhysicsStateRigidBody>> m_RigidBodySet;

    bool m_Paused = false;
    float m_SimulateSpeed = 1.0f; // time speed

    bool m_EnableGravity = true;
    float m_GravityAcceleration = 9.8f;

    bool m_CollisionDetection = true;

    std::vector<CollideCallback_t> m_CollisionCallbackSet;
};
