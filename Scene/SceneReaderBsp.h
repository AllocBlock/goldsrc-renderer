#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcBsp.h"

#include <map>
#include <memory>

class CSceneReaderBsp : public CSceneReaderBase
{
private:
    virtual std::shared_ptr<SScene> _readV() override;

    void __readBsp(std::filesystem::path vFilePath);
    void __readTextures();
    std::vector<std::shared_ptr<S3DObject>> __loadLeaf(size_t vLeafIndex);
    std::vector<std::shared_ptr<S3DObject>> __loadEntity(size_t vModelIndex);
    void __loadBspTree();
    void __loadBspPvs();
    void __correntLightmapCoords();
    void __loadSkyBox(std::filesystem::path vCurrentDir);
    bool __readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension, std::filesystem::path vCurrentDir);

    std::vector<glm::vec3> __getBspFaceVertices(size_t vFaceIndex);
    glm::vec3 __getBspFaceNormal(size_t vFaceIndex);
    std::vector<glm::vec2> __getBspFaceUnnormalizedTexCoords(size_t vFaceIndex, std::vector<glm::vec3> vVertices);
    std::pair<std::optional<size_t>, std::vector<glm::vec2>> __getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords);
    void __getBspFaceTextureSizeAndName(size_t vFaceIndex, size_t& voWidth, size_t& voHeight, std::string& voName);
    void __appendBspFaceToObject(std::shared_ptr<S3DObject> pObject, uint32_t vFaceIndex);
    std::optional<SMapEntity> __findEntity(size_t vModelIndex);

    std::shared_ptr<SScene> m_pScene = nullptr;
    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcBsp m_Bsp;
    std::map<std::string, uint32_t> m_TexNameToIndex;
    bool m_HasLightmapData = false;
};

