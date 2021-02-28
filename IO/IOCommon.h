#pragma once

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

namespace IOCommon
{
    struct SGoldSrcVec3
    {
        float X, Y, Z;

        glm::vec3 glmVec3() const;
    };

    struct SGoldSrcColor
    {
        uint8_t R, G, B;
    };

    template <typename T>
    std::vector<T> readArray(std::ifstream& vFile, uint64_t vSize)
    {
        _ASSERTE(!vFile.eof() && !vFile.fail());
        _ASSERTE(vSize % sizeof(T) == 0);
        size_t NumT = vSize / sizeof(T);
        std::vector<T> Result;
        Result.resize(NumT);
        vFile.read(reinterpret_cast<char*>(Result.data()), vSize);
        return std::move(Result);
    }

    template <typename T>
    std::vector<T> readArray(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
    {
        _ASSERTE(!vFile.eof() && !vFile.fail());
        vFile.seekg(vOffset, std::ios_base::beg);
        return IOCommon::readArray<T>(vFile, vSize);
    }
}
