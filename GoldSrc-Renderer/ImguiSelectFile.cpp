#include "ImguiSelectFile.h"

void CImguiSelectFile::draw()
{
    m_FileDialog.Display();
    if (m_FileDialog.HasSelected()) // ��ѡ��
    {
        m_IsSelecting = false;
        m_Promise.set_value(m_FileDialog.GetSelected());
        m_FileDialog.ClearSelected();
    }
    else if (m_IsSelecting && !m_FileDialog.IsOpened()) // �ѹر�
    {
        m_IsSelecting = false;
        m_Promise.set_value("");
    }
}

void CImguiSelectFile::start(std::promise<std::filesystem::path> vPromise)
{
    if (m_IsSelecting) throw std::runtime_error(u8"�ظ�����ѡ���ļ�");
    m_IsSelecting = true;
    m_Promise = std::move(vPromise);
    m_FileDialog.Open();
}

void CImguiSelectFile::abort()
{
    m_FileDialog.Close();
    m_IsSelecting = false;
    m_Promise.set_value("");
}

bool CImguiSelectFile::isOpen()
{
    return m_IsSelecting;
}

void CImguiSelectFile::setTitle(std::string vTitle)
{
    m_FileDialog.SetTitle(vTitle);
}

void CImguiSelectFile::setFilters(const std::vector<std::string>& vFilterSet)
{
    m_FileDialog.SetTypeFilters(vFilterSet);
}