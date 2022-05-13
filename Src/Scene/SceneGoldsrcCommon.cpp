#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

using namespace Common;

bool GoldSrc::readWad(std::filesystem::path vWadPath, std::filesystem::path vAdditionalSearchDir, CIOGoldsrcWad& voWad)
{
    std::filesystem::path RealWadPath;
    if (!Scene::requestFilePathUntilCancel(vWadPath, vAdditionalSearchDir, ".wad", RealWadPath))
    {
        Common::Log::log(u8"未找到WAD文件：" + vWadPath.u8string());
        return false;
    }

    Scene::reportProgress(u8"[wad]读取" + RealWadPath.u8string() + u8"文件中");
    voWad.read(RealWadPath);

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