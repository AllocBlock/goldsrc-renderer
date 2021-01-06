#pragma once

#include "IOBase.h"
#include "IOMtl.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct SObjFaceNode
{
    uint32_t VectexId = 0;
    uint32_t TexCoordId = 0;
    uint32_t NormalId = 0;
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
    std::vector<glm::vec2> TexCoords;
    std::vector<glm::vec3> Normals;
    std::vector<SObjFace> Faces;
};

class CIOObj : public CIOBase
{
public:
    CIOObj() :CIOBase() {}
    CIOObj(std::string vFileName) :CIOBase(vFileName) {}

    size_t getFaceNum() const;
    size_t getFaceNodeNum(int vFaceIndex) const;
    glm::vec3 getVertex(int vFaceIndex, int vNodeIndex) const;
    glm::vec2 getTexCoord(int vFaceIndex, int vNodeIndex) const;
    glm::vec3 getNormal(int vFaceIndex, int vNodeIndex) const;

    const std::vector<glm::vec3>& getVertices() { return m_pObj->Vertices; }
    const std::vector<glm::vec3>& getNormals() { return m_pObj->Normals; }
    const std::vector<glm::vec2>& getTexCoords() { return m_pObj->TexCoords; }
    const std::vector<SObjFace>& getFaces() { return m_pObj->Faces; }

protected:
    virtual bool _readV(std::string vFileName) override;

private:
    std::shared_ptr<SObj> m_pObj = nullptr;
    std::shared_ptr<CIOMtl> m_pMtl = nullptr;
    float m_ScaleFactor = 1.0;
};