#include "SceneCommon.h"
#include "SceneGoldsrcCommon.h"

std::vector<CIOGoldsrcWad> readWads(const std::vector<std::filesystem::path>& vWadPaths, std::function<void(std::string)> vProgressReportFunc = nullptr)
{
    std::vector<CIOGoldsrcWad> Wads(vWadPaths.size());

    for (size_t i = 0; i < vWadPaths.size(); ++i)
    {
        std::filesystem::path RealWadPath;
        if (!findFile(vWadPaths[i], "../data", RealWadPath))
        {
            globalLog(u8"未找到WAD文件：" + vWadPaths[i].u8string());
            continue;
        }
        if (vProgressReportFunc) vProgressReportFunc(u8"[wad]读取" + RealWadPath.u8string() + u8"文件中");
        Wads[i].read(RealWadPath);
    }
    return Wads;
}