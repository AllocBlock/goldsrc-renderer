#include "BoundingBox.h"

SAABB __createInvalidAABB()
{
    SAABB AABB;
    AABB.IsValid = false;
    return AABB;
}

const SAABB SAABB::InvalidAABB = __createInvalidAABB();