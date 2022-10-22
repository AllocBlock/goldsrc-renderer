#pragma once

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

const size_t BSP_MAX_NAME_LENGTH = 16;
const size_t BSP_MIPMAP_LEVEL = 4;
const size_t BSP_MAX_HULL = 4;

namespace GoldSrc
{
    struct SVec3
    {
        float X, Y, Z;

        glm::vec3 toGlm() const;
    };

    struct SColor
    {
        uint8_t R, G, B;
    };
}

namespace IO
{
    template <typename T>
    std::vector<T> readArray(std::ifstream& vFile, uint64_t vSize)
    {
        _ASSERTE(!vFile.eof() && !vFile.fail());
        _ASSERTE(vSize % sizeof(T) == 0);
        size_t NumT = vSize / sizeof(T);
        std::vector<T> Result;
        Result.resize(NumT);
        vFile.read(reinterpret_cast<char*>(Result.data()), vSize);
        return Result;
    }

    template <typename T>
    std::vector<T> readArray(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
    {
        _ASSERTE(!vFile.eof() && !vFile.fail());
        vFile.seekg(vOffset, std::ios_base::beg);
        return readArray<T>(vFile, vSize);
    }
}