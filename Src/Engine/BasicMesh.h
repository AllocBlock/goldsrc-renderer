#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace BasicMesh
{
    struct SVertex
    {
        glm::vec3 Pos;
        glm::vec3 Normal;
    };

    std::vector<SVertex> getUnitQuadEdgeSet();
    std::vector<SVertex> getUnitSphereEdgeSet();
    std::vector<SVertex> getUnitCubeEdgeSet();
    std::vector<SVertex> getUnitSphereFaceSet();
}
