#pragma once
#include "Scene.h"
#include "SceneReaderBase.h"
#include "IOGoldSrcBsp.h"

#include <map>
#include <memory>

class CSceneReaderBsp : public CSceneReaderBase
{
public:
    SScene read(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc);
private:
    void __readBsp(std::filesystem::path vFilePath);
    void __readTextures();
    void __loadLeaves();
    void __loadEntities();
    void __loadBspTreeAndPvs();

    std::vector<glm::vec3> __getBspFaceVertices(size_t vFaceIndex);
    glm::vec3 __getBspFaceNormal(size_t vFaceIndex);
    std::vector<glm::vec2> __getBspFaceUnnormalizedTexCoords(size_t vFaceIndex, std::vector<glm::vec3> vVertices);
    uint32_t __getAndAppendBspFaceLightmap(size_t vFaceIndex, const std::vector<glm::vec2>& vTexCoords, std::vector<glm::vec2>& voLightmapCoords);
    void __getBspFaceTextureSizeAndName(size_t vFaceIndex, size_t& voWidth, size_t& voHeight, std::string& voName);
    void __appendBspFaceToObject(std::shared_ptr<S3DObject> pObject, uint32_t vFaceIndex);

    const float m_SceneScale = 1.0f / 64.0f;
    CIOGoldSrcBsp m_Bsp;
    SScene m_Scene;
    std::map<std::string, uint32_t> m_TexNameToIndex;
};

