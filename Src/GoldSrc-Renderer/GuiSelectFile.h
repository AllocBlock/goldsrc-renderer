#pragma once

#include <string>
#include <future>
#include <filesystem>
#include "imfilebrowser.h"

class CGuiSelectFile
{
public:
    void draw();
    void start(std::promise<std::filesystem::path> vPromise);
    void abort();
    bool isOpen();
    void setTitle(std::string vTitle);
    void setFilters(const std::vector<std::string>& vFilterSet);

private:
    bool m_IsSelecting = false;
    UI::FileBrowser m_FileDialog;
    std::promise<std::filesystem::path> m_Promise;
};