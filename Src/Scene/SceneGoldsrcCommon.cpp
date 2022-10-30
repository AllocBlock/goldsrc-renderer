#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"
#include "Log.h"

CActor<CMeshDataGoldSrc>::Ptr GoldSrc::createActorByMeshAndTag(const CMeshDataGoldSrc& vMeshData, const std::vector<std::string> vTagSet)
{
    auto pActor = make<CActor<CMeshDataGoldSrc>>();
    auto pMesh = make<CMeshTriangleList<CMeshDataGoldSrc>>();

    pMesh->setMeshData(vMeshData);
    pActor->setMesh(pMesh);

    for (const auto& vTag : vTagSet)
        pActor->addTag(vTag);

    return pActor;
}

bool GoldSrc::readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad)
{
    auto RequestResult = Scene::requestFilePath(vWadPath, vAdditionalSearchDir, u8"��Ҫ����" + vWadPath.filename().u8string(), "wad");
    if (!RequestResult.Found)
    {
        Log::log("δ�ҵ�WAD�ļ���" + vWadPath.u8string());
        return false;
    }

    Scene::reportProgress(u8"[wad]��ȡ" + RequestResult.Data.u8string() + u8"�ļ���");
    voWad.read(RequestResult.Data);

    return true;
}

std::vector<CIOGoldsrcWad> GoldSrc::readWads(const std::vector<std::filesystem::path>& vWadPaths, std::filesystem::path vAdditionalSearchDir)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        readWad(vWadPaths[i], vAdditionalSearchDir, Wads[i]);
    }
    return Wads;
}