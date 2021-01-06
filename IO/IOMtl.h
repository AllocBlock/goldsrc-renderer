#pragma once
#include "IOBase.h"

#include <vector>
#include <glm/glm.hpp>

struct SMtlMaterial {
    std::string Name;

    glm::vec3 Ka = glm::vec3(0.2f, 0.2f, 0.2f); // r GroupName b�����ʻ���ɫ
    glm::vec3 Kd = glm::vec3(0.8f, 0.8f, 0.8f); // r GroupName b������������ɫ
    glm::vec3 Ks = glm::vec3(1.0f, 1.0f, 1.0f); // r GroupName b�����ʾ��淴��ɫ
    glm::vec3 Ke = glm::vec3(0.0f, 0.0f, 0.0f); // emissive coeficient ��ɢϵ����
    float D = 1.0f; // 0.0-1.0����͸����
    float Tr = 0.0f; // 0.0-1.0��͸���ȣ���d���⣬ʹ��ϰ������
    float Tf = 0.0f; // �˹�͸���ʣ�����
    float Ns = 0.0; // shininess�������
    float Ni = 1.0; // ������
    int IlluMode = 2; // ����ģʽ��1�����޸߹⡢���淴�䣬2�����о��淴��
    std::string Map_Ka; // ��������ͼ·��
    std::string Map_Kd; // ��������ͼ·��
    std::string Map_bump; // ��͹��ͼ·��
    std::string Bump; // ��͹��ͼ·����ͬ�ϣ�ʹ��ϰ������
};

class CIOMtl : public CIOBase
{
public:
    CIOMtl() = default;
    CIOMtl(std::string vFileName) :CIOBase(vFileName) {}
    
protected:
    virtual bool _readV(std::string vFileName) override;

private:
    std::vector<SMtlMaterial> m_Materials;
};