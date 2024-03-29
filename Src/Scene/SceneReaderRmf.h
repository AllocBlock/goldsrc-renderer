#pragma once
#include "SceneInfo.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcRmf.h"
#include "IOGoldSrcWad.h"

#include <filesystem>
#include <map>

class CSceneReaderRmf : public CSceneReaderBase
{
protected:
    virtual void _readV(ptr<SSceneInfo> voSceneInfo) override;
private:
    void __readRmf(std::filesystem::path vFilePath);
    void __readWadsAndInitTextures();
    void __readObject(ptr<SRmfObject> vpObject);
    void __readSolid(ptr<SRmfSolid> vpSolid);
    void __readSolidFace(const SRmfFace& vFace, CMeshData& vioMeshData);
    uint32_t __requestTextureIndex(std::string vTextureName);
    glm::vec2 __getTexCoord(SRmfFace vFace, glm::vec3 vVertex);

    ptr<SSceneInfo> m_pTargetSceneInfo = nullptr;
    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcRmf m_Rmf;
    std::vector<CIOGoldsrcWad> m_Wads;
    std::vector<std::string> m_UsedTextureNames;
    std::map<std::string, uint32_t> m_TexNameToIndex;
};

