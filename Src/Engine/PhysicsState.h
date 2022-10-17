#pragma once
#include "Pointer.h"
#include "Collider.h"
#include "Transform.h"

#include <glm/glm.hpp>

struct SPhysicsMaterial
{
    float StaticFriction = 0.5f;
    float Friction = 0.3f;
    float AirResistance = 0.1f;
};

struct IPhysicsState
{
    bool IsStatic = false;
    ptr<STransform> pTargetTransform = nullptr;
    SPhysicsMaterial Material;
    ICollider::Ptr pCollider = nullptr;

    glm::vec3 FrameForce = glm::vec3(0.0f);
    
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
    glm::vec3 AngularVelocity = glm::vec3(0.0f); // direction as axis, length as speed
    glm::mat3 InertiaTensor = glm::mat3(1.0f); // TODO: how to init? currently pretend it = 1 for all axis

    glm::vec3 FrameAlpha = glm::vec3(0.0f);

    void addAlpha(glm::vec3 vAlpha)
    {
        FrameAlpha += vAlpha;
    }

    void clearAlpha() { FrameAlpha = glm::vec3(0.0f); }
};

