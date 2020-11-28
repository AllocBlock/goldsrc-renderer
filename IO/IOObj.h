#pragma once

#include "IOBase.h"
#include "IOMtl.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct SObjFaceNode
{
    int VectexId = -1;
    int TexCoorIndex = -1;
    int NormalIndex = -1;
};

struct SObjFace
{
    std::vector<SObjFaceNode> Nodes;
    std::string GroupName;
    std::string SmoothGroupName;
    std::string MtlName;
};

struct SObj
{
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec2> TexCoors;
    std::vector<glm::vec3> Normals;
    std::vector<SObjFace> Faces;
    
};

class CIOObj : public CIOBase
{
public:
    CIOObj() = default;
    CIOObj(std::string vFileName) :CIOBase(vFileName) {}

    uint32_t getIndex(int faceIndex, int vectexIndex);
    glm::vec3 getVertex(int faceIndex, int index);
    glm::vec2 getTexCoor(int faceIndex, int index);
    glm::vec3 getNormal(int faceIndex, int index);

protected:
    virtual bool _readV(std::string vFileName) override;

private:
    std::shared_ptr<SObj> m_pObj = nullptr;
    std::shared_ptr<CIOMtl> m_pMtl = nullptr;
    float m_ScaleFactor = 1.0;
};