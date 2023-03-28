#pragma once
#include <glm/vec3.hpp>

namespace Visualize
{
    struct Triangle
    {
        Triangle(glm::vec3 vA, glm::vec3 vB, glm::vec3 vC): A(vA), B(vB), C(vC) {}
        glm::vec3 A;
        glm::vec3 B;
        glm::vec3 C;
    };

    struct Line
    {
        Line(glm::vec3 vStart, glm::vec3 vEnd) : Start(vStart), End(vEnd) {}
        glm::vec3 Start;
        glm::vec3 End;
    };

    using Point = glm::vec3;
}
