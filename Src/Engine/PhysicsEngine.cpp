#include "PhysicsEngine.h"

#include <iostream>

glm::vec3 __normalizeSafe(glm::vec3 v)
{
    if (glm::length(v) < 1e-5) return glm::vec3(0.0f);
    return glm::normalize(v);
}

glm::vec3 __calcTorque(glm::vec3 vCenter, glm::vec3 vForce, glm::vec3 vPointOfAction)
{
    return glm::cross(vForce, vPointOfAction - vCenter);
}

void CPhysicsEngine::update(float vDeltaTime) const
{
    if (m_Paused) return;
    
    // 1. collision detection
    size_t RigidNum = m_RigidBodySet.size();
    std::vector<std::vector<glm::vec3>> CollideForceSet(RigidNum);
    std::vector<std::vector<glm::vec3>> CollideAlphaSet(RigidNum);

    // TODO: use accelerate data struct too speed up
    if (m_CollisionDetection)
    {
        for (size_t i = 0; i < RigidNum; ++i)
        {
            for (size_t k = i + 1; k < RigidNum; ++k)
            {
                glm::vec3 Pos, Normal;
                float Depth;
                if (collide(m_RigidBodySet[i]->pCollider, m_RigidBodySet[k]->pCollider, Pos, Normal, Depth))
                {
                    for (const auto& Func : m_CollisionCallbackSet)
                        Func(Pos, Normal);

                    std::cout << "Hit " << i << " with " << k << "\n";
                    // 2. solve constraints
                    // TODO: Penalty Force method, use solving constraint later
                    float PenaltyForceIntensity = 40.0f * pow(Depth, 2);
                    CollideForceSet[i].emplace_back(Normal * PenaltyForceIntensity);
                    CollideForceSet[k].emplace_back(-Normal * PenaltyForceIntensity);

                    float J = 1.0; // FIXME: use real inertia tensor
                    CollideAlphaSet[i].emplace_back(__calcTorque(m_RigidBodySet[i]->pTargetTransform->getAbsoluteTranslate(), Normal * PenaltyForceIntensity, Pos) / J);
                    CollideAlphaSet[k].emplace_back(__calcTorque(m_RigidBodySet[k]->pTargetTransform->getAbsoluteTranslate(), -Normal * PenaltyForceIntensity, Pos) / J);
                }
            }
        }
    }

    float dt = m_SimulateSpeed * vDeltaTime;
    
    for (size_t i = 0; i < RigidNum; ++i)
    {
        ptr<SPhysicsStateRigidBody> pRigid = m_RigidBodySet[i];
        if (pRigid->IsStatic) continue;

        // 3. calculate force and alpha
        glm::vec3 F = pRigid->FrameForce;
        pRigid->clearForce();
        for (const glm::vec3& TempF : CollideForceSet[i])
            F += TempF;

        glm::vec3 Alpha = pRigid->FrameAlpha;
        pRigid->clearAlpha();
        for (const glm::vec3& TempAlpha : CollideAlphaSet[i])
            Alpha += TempAlpha;

        // gravity
        if (m_EnableGravity && pRigid->HasGravity)
        {
            F += glm::vec3(0.0, 0.0, -1.0) * m_GravityAcceleration * pRigid->Mass;
        }

        // air resistance, F = 1/2C¦ÑSV^2
        F += -__normalizeSafe(pRigid->Velocity) * glm::pow(glm::length(pRigid->Velocity), 2.0f) * pRigid->Material.AirResistance;
        Alpha += -__normalizeSafe(pRigid->AngularVelocity) * glm::pow(glm::length(pRigid->AngularVelocity), 2.0f) * pRigid->Material.AirResistance;
        
        // 4. update velocity, position
        // FIXME: now it's explicit euler, replace by mixed euler later

        glm::vec3 A = F / pRigid->Mass;
        // dv = a*dt
        glm::vec3 DeltaSpeed = A * dt;
        // dx = v*dt + 1/2 * a * dt^2
        glm::vec3 DeltaPos = pRigid->Velocity * dt + 0.5f * A * dt * dt;
            
        pRigid->Velocity += DeltaSpeed;
        pRigid->pTargetTransform->Translate += DeltaPos;

        // 5. update rotation
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
}
