#include "BasicMesh.h"

#include <array>
#include <stdexcept>

enum class EPrimitiveType
{
    FACE,
    EDGE,
    POINT
};

void __generateSphereHelper(EPrimitiveType vType, std::vector<BasicMesh::SVertex>& vioVertexSet, const std::array<glm::vec3, 3>& vFace, int vRecursiveDepth)
{
    if (vRecursiveDepth == 0)
    {
        switch (vType)
        {
        case EPrimitiveType::FACE:
        {
            // Flat
            //const glm::vec3 Normal = glm::normalize(glm::cross(glm::vec3(vFace[1] - vFace[0]), glm::vec3(vFace[2] - vFace[0])));
            /*for (int i = 0; i < 3; ++i)
                vioVertexSet.push_back({ vFace[i], Normal });*/

            // Per vertex
            for (int i = 0; i < 3; ++i)
                vioVertexSet.push_back({ vFace[i], vFace[i] });
            break;
        }
        case EPrimitiveType::EDGE:
        {
            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& P1 = vFace[i];
                const glm::vec3& P2 = vFace[(i + 1) % 3];
                const glm::vec3 Normal = glm::normalize(P1 + P2);
                vioVertexSet.push_back({ P1, Normal });
                vioVertexSet.push_back({ P2, Normal });
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
        __generateSphereHelper(vType, vioVertexSet, { A, D, F }, vRecursiveDepth - 1);
        __generateSphereHelper(vType, vioVertexSet, { D, B, E }, vRecursiveDepth - 1);
        __generateSphereHelper(vType, vioVertexSet, { F, E, C }, vRecursiveDepth - 1);
        __generateSphereHelper(vType, vioVertexSet, { D, E, F }, vRecursiveDepth - 1);
    }
}

std::vector<BasicMesh::SVertex> __generateSphere(EPrimitiveType vType, int vRecursiveDepth = 3)
{
    glm::vec3 A = { -2.0 * sqrt(2) / 3.0, 0, -1.0 / 3.0 };
    glm::vec3 B = { sqrt(2) / 3.0, sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 C = { sqrt(2) / 3.0, -sqrt(6) / 3.0, -1.0 / 3.0 };
    glm::vec3 D = { 0, 0, 1 };
    std::vector<BasicMesh::SVertex> VertexSet;
    switch (vType)
    {
    case EPrimitiveType::FACE:
    case EPrimitiveType::EDGE:
    {
        __generateSphereHelper(vType, VertexSet, { A, B, C }, vRecursiveDepth);
        __generateSphereHelper(vType, VertexSet, { D, A, C }, vRecursiveDepth);
        __generateSphereHelper(vType, VertexSet, { D, C, B }, vRecursiveDepth);
        __generateSphereHelper(vType, VertexSet, { D, B, A }, vRecursiveDepth);
        break;
    }
    case EPrimitiveType::POINT:
    default:
        throw std::runtime_error("Unknown primitive type");
    }
    
    return VertexSet;
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

std::vector<BasicMesh::SVertex> BasicMesh::getUnitQuadFaceSet()
{
    return {
        {{ 1,  1, 0}, {0, 0, 1}},
        {{ -1, 1, 0}, {0, 0, 1}},
        {{ -1, -1, 0}, {0, 0, 1}},
        {{ 1,  1, 0}, {0, 0, 1}},
        {{-1, -1, 0}, {0, 0, 1}},
        {{ 1, -1, 0}, {0, 0, 1}},
    };
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitCubeFaceSet()
{
    /*
    *   4------5      y
    *  /|     /|      |
    * 0------1 |      |
    * | 7----|-6      -----x
    * |/     |/      /
    * 3------2      z
    */
    std::array<glm::vec3, 8> VertexSet =
    {
        glm::vec3(-1,  1,  1),
        glm::vec3( 1,  1,  1),
        glm::vec3( 1, -1,  1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3( 1,  1, -1),
        glm::vec3( 1, -1, -1),
        glm::vec3(-1, -1, -1),
    };

    const std::array<size_t, 36> IndexSet =
    {
        0, 2, 1, 0, 3, 2, // front
        5, 7, 4, 5, 6, 7, // back
        4, 1, 5, 4, 0, 1, // up
        3, 6, 2, 3, 7, 6, // down
        4, 3, 0, 4, 7, 3, // left
        1, 6, 5, 1, 2, 6, // right
    };

    const std::array<glm::vec3, 6> NormalSet =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    std::vector<BasicMesh::SVertex> TriangleVertexSet(IndexSet.size());
    for (size_t i = 0; i < IndexSet.size(); ++i)
    {
        size_t Index = IndexSet[i];
        TriangleVertexSet[i].Pos = VertexSet[Index];
        TriangleVertexSet[i].Normal = NormalSet[i / 6];
    }

    return TriangleVertexSet;
}

std::vector<BasicMesh::SVertex> BasicMesh::getUnitSphereFaceSet()
{
    return __generateSphere(EPrimitiveType::FACE);
}