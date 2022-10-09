#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcRmf.h"
#include "IOGoldSrcWad.h"

#include <filesystem>
#include <map>

class CSceneReaderRmf : public CSceneReaderBase
{
protected:
    virtual ptr<SScene> _readV() override;
private:
    void __readRmf(std::filesystem::path vFilePath);
    void __readWadsAndInitTextures();
    void __readObject(ptr<SRmfObject> vpObject);
    void __readSolid(ptr<SRmfSolid> vpSolid);
    void __readSolidFace(const SRmfFace& vFace, ptr<CMeshDataGoldSrc> vopObject);
    uint32_t __requestTextureIndex(std::string vTextureName);
    glm::vec2 __getTexCoord(SRmfFace vFace, glm::vec3 vVertex);

    ptr<SScene> m_pScene = nullptr;
    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcRmf m_Rmf;
    std::vector<CIOGoldsrcWad> m_Wads;
    std::vector<std::string> m_UsedTextureNames;
    std::map<std::string, uint32_t> m_TexNameToIndex;
};

