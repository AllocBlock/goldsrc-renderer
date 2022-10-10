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

    void add(ptr<SPhysicsStateRigidBody> vRigid)
    {
        _ASSERTE(vRigid);
        m_RigidBodySet.emplace_back(vRigid);
    }
    void clear() { m_RigidBodySet.clear(); }
    void update(float vDeltaTime)
    {
        float dt = m_SimulateSpeed * vDeltaTime;

        for (ptr<SPhysicsStateRigidBody> pRigid : m_RigidBodySet)
        {
            if (pRigid->IsStatic) continue;

            // 1. calculate force
            glm::vec3 F = pRigid->FrameForce;
            if (pRigid->HasGravity)
            {
                F += glm::vec3(0.0, 0.0, -1.0) * m_GravityAcceleration * pRigid->Mass;
            }

            pRigid->clearForce();

            // 2. update velocity and position
            // explicit euler
            glm::vec3 A = F / pRigid->Mass;
            // dv = a*dt
            glm::vec3 DeltaSpeed = A * dt;
            // dx = v*dt + 1/2 * a * dt^2
            glm::vec3 DeltaPos = pRigid->Velocity * dt + 0.5f * A * dt * dt;
            
            pRigid->Velocity += DeltaSpeed;
            pRigid->pTargetTransform->Translate += DeltaPos;
        }

        // 3. collision detection

        // 4. solve constraints
    }

private:
    std::vector<ptr<SPhysicsStateRigidBody>> m_RigidBodySet;

    float m_SimulateSpeed = 1.0f; // time speed
    float m_GravityAcceleration = 9.8f;
};

