#include "PhysicsEngine.h"

glm::vec3 __normalizeSafe(glm::vec3 v)
{
    if (glm::length(v) < 1e-5) return glm::vec3(0.0f);
    return glm::normalize(v);
}

void CPhysicsEngine::update(float vDeltaTime) const
{
    if (m_Paused) return;

    float dt = m_SimulateSpeed * vDeltaTime;

    for (ptr<SPhysicsStateRigidBody> pRigid : m_RigidBodySet)
    {
        if (pRigid->IsStatic) continue;

        // 1. calculate force and alpha
        glm::vec3 Alpha = pRigid->FrameAlpha;
        pRigid->clearAlpha();

        glm::vec3 F = pRigid->FrameForce;

        // gravity
        if (m_EnableGravity & pRigid->HasGravity)
        {
            F += glm::vec3(0.0, 0.0, -1.0) * m_GravityAcceleration * pRigid->Mass;
        }
        // TODO: calculate alpha

        // air resistance, F = 1/2C¦ÑSV^2
        F += -__normalizeSafe(pRigid->Velocity) * glm::pow(glm::length(pRigid->Velocity), 2.0f) * pRigid->Material.AirResistance;
        Alpha += -__normalizeSafe(pRigid->AngularVelocity) * glm::pow(glm::length(pRigid->AngularVelocity), 2.0f) * pRigid->Material.AirResistance;

        pRigid->clearForce();

        // 2. update velocity, position
        // FIXME: now it's explicit euler, replace by mixed euler later

        glm::vec3 A = F / pRigid->Mass;
        // dv = a*dt
        glm::vec3 DeltaSpeed = A * dt;
        // dx = v*dt + 1/2 * a * dt^2
        glm::vec3 DeltaPos = pRigid->Velocity * dt + 0.5f * A * dt * dt;
            
        pRigid->Velocity += DeltaSpeed;
        pRigid->pTargetTransform->Translate += DeltaPos;

        // 3. update rotation
        glm::vec3 DeltaAngularVelocity = Alpha * dt;
        glm::vec3 DeltaAngular = pRigid->AngularVelocity * dt + 0.5f * Alpha * dt * dt;

        pRigid->Velocity += DeltaSpeed;

        glm::quat DeltaRotation;
        float HalfTheta = glm::length(DeltaAngular) * 0.5;
        if (HalfTheta < 1e-5)
            DeltaRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        else
            DeltaRotation = glm::quat(glm::cos(HalfTheta), glm::sin(HalfTheta) * glm::normalize(DeltaAngular));
        
        glm::quat NewRotation = DeltaRotation * pRigid->pTargetTransform->Rotate.getQuaternion();
        pRigid->pTargetTransform->Rotate.setByQuaternion(NewRotation);

        pRigid->AngularVelocity += DeltaAngularVelocity;
    }

    // 4. collision detection

    // 5. solve constraints
}
