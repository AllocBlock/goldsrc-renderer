#pragma once
#include <glm/glm.hpp>

struct SMaterialPBR
{
    glm::vec4 Albedo = glm::vec4(1.0f);
    glm::vec4 OMR = glm::vec4(0.0f); // Occlusion, Metallic, Roughness
    int ColorIdx = -1;
    int SpecularIdx = -1;
    int NormalIdx = -1;
    int Padding = 0;
};

