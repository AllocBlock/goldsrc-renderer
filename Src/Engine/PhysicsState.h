#pragma once
#include "Pointer.h"
#include "Collider.h"
#include "Transform.h"

#include <glm/glm.hpp>

struct SPhysicsMaterial
{
    float StaticFriction = 0.5f;
    float Friction = 0.3f;
};

struct IPhysicsState
{
    bool IsStatic = false;
    ptr<STransform> pTargetTransform = nullptr;
    SPhysicsMaterial Material;
    ICollider::Ptr Collider;

    glm::vec3 FrameForce = glm::vec3(0.0f);

    // TODO: add force apply position, for rotation change
    void addForce(glm::vec3 vForce)
    {
        FrameForce += vForce;
    }

    void clearForce() { FrameForce = glm::vec3(0.0f); }
};


struct SPhysicsStateRigidBody : IPhysicsState
{
    float Mass = 1.0f;
    bool HasGravity = true;
    glm::vec3 Velocity = glm::vec3(0.0f);
};

