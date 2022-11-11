#pragma once
#include <string>
#include <vector>

namespace ShaderErrorParser
{
    struct SShaderError
    {
        size_t LineNum = 0;
        std::string ErrorCode = "";
        std::string ErrorMsg = "";
    };

    std::vector<SShaderError> parse(const std::string& vErrorStr);
    std::string parseAndFormat(const std::string& vErrorStr);
};
