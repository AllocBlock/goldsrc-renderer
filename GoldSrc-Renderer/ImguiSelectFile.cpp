#include "ImguiSelectFile.h"

void CImguiSelectFile::draw()
{
    m_FileDialog.Display();
    if (m_FileDialog.HasSelected()) // 已选择
    {
        m_pCallback(m_FileDialog.GetSelected());
        m_FileDialog.ClearSelected();
    }
    else if (m_IsSelecting && !m_FileDialog.IsOpened()) // 已关闭
    {
        m_IsSelecting = false;
        m_pCallback("");
    }
}

void CImguiSelectFile::start(std::function<void(std::filesystem::path)> vCallback)
{
    _ASSERTE(vCallback);
    if (m_IsSelecting) throw std::runtime_error(u8"重复调用选择文件");
    m_IsSelecting = true;
    m_pCallback = vCallback;
    m_FileDialog.Open();
}

void CImguiSelectFile::abort()
{
    m_FileDialog.Close();
    m_IsSelecting = false;
    m_pCallback = nullptr;
}

void CImguiSelectFile::setTitle(std::string vTitle)
{
    m_FileDialog.SetTitle(vTitle);
}

void CImguiSelectFile::setFilters(const std::vector<std::string>& vFilterSet)
{
    m_FileDialog.SetTypeFilters(vFilterSet);
}