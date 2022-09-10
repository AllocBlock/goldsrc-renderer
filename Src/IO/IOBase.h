#pragma once
#include <string>
#include <vector>
#include <filesystem>

const size_t MAX_LINE_SIZE = 1024;

class CIOBase
{
public:
    CIOBase();
    CIOBase(std::filesystem::path vFilePath);
    virtual ~CIOBase() = default;
    bool read();
    bool read(std::filesystem::path vFilePath);

    static bool isWhiteSpace(char vChar);
    static std::string trimString(std::string vString);
    static std::vector<std::string> splitString(std::string vString, char vSpliter = ' ');
    static std::string toUpperCase(std::string vString);

protected:
    virtual bool _readV(std::filesystem::path vFilePath) = 0;
    
    std::filesystem::path m_FilePath;
};