#pragma once
#include "SceneInfoGoldSrc.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcBsp.h"
#include "Mesh.h"

#include <map>
#include <memory>

class CSceneReaderBsp : public CSceneReaderBase
{
private:
    virtual ptr<SSceneInfoGoldSrc> _readV() override;

    void __readBsp(std::filesystem::path vFilePath);
    void __readTextures();
    std::vector<CActor::Ptr> __loadLeaf(size_t vLeafIndex);
    std::vector<CActor::Ptr> __loadEntity(size_t vModelIndex);
    void __loadBspTree();
    void __loadBspPvs();
    void __correntLightmapCoords();
    void __loadSkyBox(std::filesystem::path vCurrentDir);
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension, std::filesystem::path vCurrentDir);
    void __loadPointEntities();
    void __appendCube(glm::vec3 vOrigin, float vSize, CMeshData& vioMeshData);

    std::vector<glm::vec3> __getBspFaceVertices(size_t vFaceIndex);
    glm::vec3 __getBspFaceNormal(size_t vFaceIndex);
    std::vector<glm::vec2> __getBspFaceUnnormalizedTexCoords(size_t vFaceIndex, std::vector<glm::vec3> vVertices);
    std::pair<uint32_t, std::vector<glm::vec2>> __getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords);
    void __getBspFaceTextureSizeAndName(size_t vFaceIndex, size_t& voWidth, size_t& voHeight, std::string& voName);
    void __appendBspFaceToObject(CMeshData& vioMeshData, uint32_t vFaceIndex);
    std::optional<SMapEntity> __findEntity(size_t vModelIndex);

    ptr<SSceneInfoGoldSrc> m_pSceneInfo = nullptr;
    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcBsp m_Bsp;
    std::map<std::string, uint32_t> m_TexNameToIndex;
    bool m_HasLightmapData = false;
};

