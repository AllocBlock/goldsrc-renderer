#include "IOCommon.h"

using namespace Common;

glm::vec3 GoldSrc::SVec3::toGlm() const
{
    return glm::vec3(X, Y, Z);
}