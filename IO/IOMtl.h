#pragma once
#include "IOBase.h"

#include <vector>
#include <glm/glm.hpp>

struct SMtlMaterial {
    std::string Name;

    glm::vec3 Ka = glm::vec3(0.2f, 0.2f, 0.2f); // r GroupName b，材质环境色
    glm::vec3 Kd = glm::vec3(0.8f, 0.8f, 0.8f); // r GroupName b，材质漫反射色
    glm::vec3 Ks = glm::vec3(1.0f, 1.0f, 1.0f); // r GroupName b，材质镜面反射色
    glm::vec3 Ke = glm::vec3(0.0f, 0.0f, 0.0f); // emissive coeficient 扩散系数？
    float D = 1.0f; // 0.0-1.0，不透明度
    float Tr = 0.0f; // 0.0-1.0，透明度，和d互斥，使用习惯问题
    float Tf = 0.0f; // 滤光透射率？？？
    float Ns = 0.0; // shininess，反光度
    float Ni = 1.0; // 折射率
    int IlluMode = 2; // 光照模式，1代表无高光、镜面反射，2代表有镜面反射
    std::string Map_Ka; // 环境光贴图路径
    std::string Map_Kd; // 漫反射贴图路径
    std::string Map_bump; // 凹凸贴图路径
    std::string Bump; // 凹凸贴图路径，同上，使用习惯问题
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