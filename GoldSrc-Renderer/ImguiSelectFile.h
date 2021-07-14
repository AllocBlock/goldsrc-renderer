#pragma once

#include <string>
#include <future>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imfilebrowser.h"

class CImguiSelectFile
{
public:
    void draw();
    void start(std::function<void(std::filesystem::path)> vCallback);
    void abort();
    void setTitle(std::string vTitle);
    void setFilters(const std::vector<std::string>& vFilterSet);

private:
    bool m_IsSelecting = false;
    ImGui::FileBrowser m_FileDialog;
    std::promise<std::filesystem::path> m_Promise;
    std::function<void(std::filesystem::path)> m_pCallback = nullptr;
};