#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcRmf.h"
#include "IOGoldSrcWad.h"

#include <filesystem>
#include <map>

class CSceneReaderRmf : public CSceneReaderBase
{
public:
    SScene read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc);
private:
    void __readRmf(std::filesystem::path vFilePath);
    void __readWadsAndInitTextures();
    void __readObject(std::shared_ptr<SRmfObject> vpObject);
    void __readSolid(std::shared_ptr<SRmfSolid> vpSolid);
    void __readSolidFace(const SRmfFace& vFace, std::shared_ptr<S3DObject> vopObject);
    uint32_t __requestTextureIndex(std::string vTextureName);
    glm::vec2 __getTexCoord(SRmfFace vFace, glm::vec3 vVertex);

    SScene m_Scene;
    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcRmf m_Rmf;
    std::vector<CIOGoldsrcWad> m_Wads;
    std::vector<std::string> m_UsedTextureNames;
    std::map<std::string, uint32_t> m_TexNameToIndex;
};

