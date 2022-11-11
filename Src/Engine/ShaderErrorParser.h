#pragma once
#include "Common.h"
#include <string>
#include <vector>
#include <regex>

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
