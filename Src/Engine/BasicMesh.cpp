#include "BasicMesh.h"

#include <array>
#include <stdexcept>

enum class EPrimitiveType
{
    FACE,
    EDGE,
    POINT
};

void __generateSphereHelper(EPrimitiveType vType, std::vector<BasicMesh::SVertex>& vioResult, std::array<glm::vec3, 3> vFace, int vRecursiveDepth)
{
    if (vRecursiveDepth == 0)
    {
        switch (vType)
        {
        case EPrimitiveType::FACE:
        {
            const glm::vec3 Normal = glm::normalize(glm::cross(glm::vec3(vFace[1] - vFace[0]), glm::vec3(vFace[2] - vFace[0])));
            for (int i = 0; i < 3; ++i)
                vioResult.push_back({ vFace[i], Normal });
            break;
        }
        case EPrimitiveType::EDGE:
        {
            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& P1 = vFace[i];
                const glm::vec3& P2 = vFace[(i + 1) % 3];
                const glm::vec3 Normal = glm::normalize(P1 + P2);
                vioResult.push_back({ P1, Normal });
                vioResult.push_back({ P2, Normal });
            }
            break;
        }
        case EPrimitiveType::POINT:
        {
            break;
        }
        default:
            throw std::runtime_error("Unknown primitive type");
        }
    }
    else
    {
        const glm::vec3& A = vFace[0];
        const glm::vec3& B = vFace[1];
        const glm::vec3& C = vFace[2];
        const glm::vec3 D = glm::normalize(vFace[0] + vFace[1]);
        const glm::vec3 E = glm::normalize(vFace[1] + vFace[2]);
        const glm::vec3 F = glm::normalize(vFace[2] + vFace[0]);
        __generateSphereHelper(EPrimitiveType::EDGE, vioResult, { A, D, F }, vRecursiveDepth - 1);
        __generateSphereHelper(EPrimitiveType::EDGE, vioResult, { D, B, E }, vRecursiveDepth - 1);
        __generateSphereHelper(EPrimitiveType::EDGE, vioResult, { F, E, C }, vRecursiveDepth - 1);
        __generateSphereHelper(EPrimitiveType::EDGE, vioResult, { D, E, F }, vRecursiveDepth - 1);
    }
}

std::vector<BasicMesh::SVertex> __generateSphere(EPrimitiveType vType, int vRecursiveDepth = 3)
{
    glm::vec3 A = { -2.0 * sqrt(2) / 3.0, 0, -1.0 / 3.0 };
    glm::vec3 B = { sqrt(2) / 3.0, sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 C = { sqrt(2) / 3.0, -sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 D = { 0, 0, 1 };
    std::vector<BasicMesh::SVertex> Result;
    switch (vType)
    {
    case EPrimitiveType::FACE:
    {
        __generateSphereHelper(EPrimitiveType::FACE, Result, { A, B, C }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::FACE, Result, { D, A, C }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::FACE, Result, { D, C, B }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::FACE, Result, { D, B, A }, vRecursiveDepth);
        break;
    }
    case EPrimitiveType::EDGE:
    {
        // FIXME: this generate duplicate edges, how to fix?
        __generateSphereHelper(EPrimitiveType::EDGE, Result, { A, B, C }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::EDGE, Result, { D, A, C }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::EDGE, Result, { D, C, B }, vRecursiveDepth);
        __generateSphereHelper(EPrimitiveType::EDGE, Result, { D, B, A }, vRecursiveDepth);
        break;
    }
    case EPrimitiveType::POINT:
    {
        break;
    }
    default:
        throw std::runtime_error("Unknown primitive type");
    }
    
    return Result;
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitQuadEdgeSet()
{
    return {
        {{ 1,  1, 0}, {0, 0, 1}},
        {{ 1, -1, 0}, {0, 0, 1}},
        {{ 1, -1, 0}, {0, 0, 1}},
        {{-1, -1, 0}, {0, 0, 1}},
        {{-1, -1, 0}, {0, 0, 1}},
        {{-1,  1, 0}, {0, 0, 1}},
        {{-1,  1, 0}, {0, 0, 1}},
        {{ 1,  1, 0}, {0, 0, 1}},
    };
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitCubeEdgeSet()
{
    const float HalfSqrt2 = sqrt(2) * 0.5;
    return {
        {{ 1,  1, -1}, { HalfSqrt2,  HalfSqrt2, 0}},
        {{ 1,  1,  1}, { HalfSqrt2,  HalfSqrt2, 0}},
        {{-1,  1, -1}, {-HalfSqrt2,  HalfSqrt2, 0}},
        {{-1,  1,  1}, {-HalfSqrt2,  HalfSqrt2, 0}},
        {{ 1, -1, -1}, { HalfSqrt2, -HalfSqrt2, 0}},
        {{ 1, -1,  1}, { HalfSqrt2, -HalfSqrt2, 0}},
        {{-1, -1, -1}, {-HalfSqrt2, -HalfSqrt2, 0}},
        {{-1, -1,  1}, {-HalfSqrt2, -HalfSqrt2, 0}},

        {{-1,  1,  1}, {0,  HalfSqrt2,  HalfSqrt2}},
        {{ 1,  1,  1}, {0,  HalfSqrt2,  HalfSqrt2}},
        {{-1, -1,  1}, {0, -HalfSqrt2,  HalfSqrt2}},
        {{ 1, -1,  1}, {0, -HalfSqrt2,  HalfSqrt2}},
        {{-1,  1, -1}, {0,  HalfSqrt2, -HalfSqrt2}},
        {{ 1,  1, -1}, {0,  HalfSqrt2, -HalfSqrt2}},
        {{-1, -1, -1}, {0, -HalfSqrt2, -HalfSqrt2}},
        {{ 1, -1, -1}, {0, -HalfSqrt2, -HalfSqrt2}},

        {{ 1, -1,  1}, { HalfSqrt2, 0,  HalfSqrt2}},
        {{ 1,  1,  1}, { HalfSqrt2, 0,  HalfSqrt2}},
        {{-1, -1,  1}, {-HalfSqrt2, 0,  HalfSqrt2}},
        {{-1,  1,  1}, {-HalfSqrt2, 0,  HalfSqrt2}},
        {{ 1, -1, -1}, { HalfSqrt2, 0, -HalfSqrt2}},
        {{ 1,  1, -1}, { HalfSqrt2, 0, -HalfSqrt2}},
        {{-1, -1, -1}, {-HalfSqrt2, 0, -HalfSqrt2}},
        {{-1,  1, -1}, {-HalfSqrt2, 0, -HalfSqrt2}},
    };
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitSphereEdgeSet()
{
    return __generateSphere(EPrimitiveType::EDGE);
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitSphereFaceSet()
{
    return __generateSphere(EPrimitiveType::FACE);
}