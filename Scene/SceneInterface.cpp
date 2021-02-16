#include "SceneInterface.h"
#include <string>
#include <functional>
#include <algorithm>

using ReadFunc_t = std::function<SScene(std::filesystem::path, std::function<void(std::string)>)>;
std::map<std::string, ReadFunc_t> g_RegistedList;

template <typename T>
struct SRegister
{
    SRegister() = delete;
    SRegister(std::string vType)
    {
        std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
        g_RegistedList[vType] = [](std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc) -> SScene
        {
            T Reader;
            return Reader.read(vFilePath, vProgressReportFunc);
        };
    }
};

#define REGISTER_FILE(CLASS, TYPE) SRegister<CLASS> g_Register##CLASS##(TYPE);

REGISTER_FILE(CSceneReaderObj, "obj");
REGISTER_FILE(CSceneReaderMap, "map");
REGISTER_FILE(CSceneReaderBsp, "bsp");

SScene SceneReader::read(std::string vType, std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    std::transform(vType.begin(), vType.end(), vType.begin(), ::tolower);
    if (g_RegistedList.count(vType) == 0)
        throw std::runtime_error(u8"δ֪���ļ�����");
    else
        return g_RegistedList[vType](vFilePath, vProgressReportFunc);
}