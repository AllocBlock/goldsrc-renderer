#include "GuiSelectFile.h"
#include "Gui.h"
#include "imfilebrowser.h"

void CGuiSelectFile::draw()
{
    m_FileDialog.Display();
    if (m_FileDialog.HasSelected()) // 已选择
    {
        m_IsSelecting = false;
        m_Promise.set_value(m_FileDialog.GetSelected());
        m_FileDialog.ClearSelected();
    }
    else if (m_IsSelecting && !m_FileDialog.IsOpened()) // 已关闭
    {
        m_IsSelecting = false;
        m_Promise.set_value("");
    }
}

void CGuiSelectFile::start(std::promise<std::filesystem::path> vPromise)
{
    if (m_IsSelecting) throw std::runtime_error(u8"重复调用选择文件");
    m_IsSelecting = true;
    m_Promise = std::move(vPromise);
    m_FileDialog.Open();
}

void CGuiSelectFile::abort()
{
    m_FileDialog.Close();
    m_IsSelecting = false;
    m_Promise.set_value("");
}

bool CGuiSelectFile::isOpen()
{
    return m_IsSelecting;
}

void CGuiSelectFile::setTitle(std::string vTitle)
{
    m_FileDialog.SetTitle(vTitle);
}

void CGuiSelectFile::setFilters(const std::vector<std::string>& vFilterSet)
{
    m_FileDialog.SetTypeFilters(vFilterSet);
}