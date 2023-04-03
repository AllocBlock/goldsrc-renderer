#pragma once

#include "Pointer.h"
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
    CIOObj() : CIOBase() {}
    CIOObj(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    size_t getFaceNum() const;
    size_t getFaceNodeNum(size_t vFaceIndex) const;
    bool dumpVertex(size_t vFaceIndex, size_t vNodeIndex, glm::vec3& voVertex) const;
    bool dumpTexCoord(size_t vFaceIndex, size_t vNodeIndex, glm::vec2& voTexCoord) const;
    bool dumpNormal(size_t vFaceIndex, size_t vNodeIndex, glm::vec3& voNormal) const;

    const std::vector<glm::vec3>& getVertices() { return m_pObj->Vertices; }
    const std::vector<glm::vec3>& getNormals() { return m_pObj->Normals; }
    const std::vector<glm::vec2>& getTexCoords() { return m_pObj->TexCoords; }
    const std::vector<SObjFace>& getFaces() { return m_pObj->Faces; }

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    ptr<SObj> m_pObj = nullptr;
    ptr<CIOMtl> m_pMtl = nullptr;
};