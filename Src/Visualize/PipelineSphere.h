#pragma once
#include "PipelineVisualizePrimitiveInstanced.h"

class CPipelineSphere : public CPipelineVisualizePrimitiveInstanced
{
public:
    uint32_t SphereRecursiveDepth = 3;
protected:
    virtual std::vector<SPointData> _createPrimitive() override
    {
        glm::vec3 A = { -2.0 * sqrt(2) / 3.0, 0, -1.0 / 3.0 };
        glm::vec3 B = { sqrt(2) / 3.0, sqrt(6) / 3.0, -1.0 / 3.0 };
        glm::vec3 C = { sqrt(2) / 3.0, -sqrt(6) / 3.0, -1.0 / 3.0 };
        glm::vec3 D = { 0, 0, 1 };

        std::vector<SPointData> VertexSet;

        uint32_t RecursiveDepth = CPipelineSphere::SphereRecursiveDepth;
        __generateSphereHelper(VertexSet, { A, B, C }, RecursiveDepth);
        __generateSphereHelper(VertexSet, { D, A, C }, RecursiveDepth);
        __generateSphereHelper(VertexSet, { D, C, B }, RecursiveDepth);
        __generateSphereHelper(VertexSet, { D, B, A }, RecursiveDepth);
        
        return VertexSet;
    }

private:
    static void __generateSphereHelper(std::vector<SPointData>& vioVertexSet, const std::array<glm::vec3, 3>& vFace, uint32_t vLeftDepth)
    {
        if (vLeftDepth == 0)
        {
            for (int i = 0; i < 3; ++i)
                vioVertexSet.push_back({ vFace[i], vFace[i] });
        }
        else
        {
            const glm::vec3& A = vFace[0];
            const glm::vec3& B = vFace[1];
            const glm::vec3& C = vFace[2];
            const glm::vec3 D = glm::normalize(vFace[0] + vFace[1]);
            const glm::vec3 E = glm::normalize(vFace[1] + vFace[2]);
            const glm::vec3 F = glm::normalize(vFace[2] + vFace[0]);
            __generateSphereHelper(vioVertexSet, { A, D, F }, vLeftDepth - 1);
            __generateSphereHelper(vioVertexSet, { D, B, E }, vLeftDepth - 1);
            __generateSphereHelper(vioVertexSet, { F, E, C }, vLeftDepth - 1);
            __generateSphereHelper(vioVertexSet, { D, E, F }, vLeftDepth - 1);
        }
    }
};