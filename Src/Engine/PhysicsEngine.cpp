#include "PhysicsEngine.h"

void CPhysicsEngine::update(float vDeltaTime) const
{
    if (m_Paused) return;

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
