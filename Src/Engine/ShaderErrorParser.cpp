#include "ShaderErrorParser.h"
#include "Common.h"

#include <regex>

std::vector<GlslShaderErrorParser::SShaderError> GlslShaderErrorParser::parse(const std::string& vErrorStr)
{
    auto LineSet = Common::split(vErrorStr, "\n");

    // remove first line, as it's the shader path
    LineSet.erase(LineSet.begin());

    // start matching
    // ERROR: $FILE_PATH$:$LINE_NUM$: '$ERROR_CODE$' : $ERROR_MESSAGE$
    const std::regex RePattern(R"(ERROR:\s*.*?:(\d*?):\s*'(.*?)'\s*:\s*(.*?))");
    std::smatch MatchResult;
    std::vector<SShaderError> ResultSet;
    for (const auto& Line : LineSet)
    {
        if (std::regex_match(Line, MatchResult, RePattern))
        {
            SShaderError Error;
            Error.LineNum = std::stoi(MatchResult[1]);
            Error.ErrorCode = MatchResult[2];
            Error.ErrorMsg = MatchResult[3];
            ResultSet.emplace_back(Error);
        }
    }
    return ResultSet;
}

size_t __getDigitNum(size_t vNumber)
{
    if (vNumber == 0) return 1;
    size_t Num = 0;
    while (vNumber /= 10) Num++;
    return Num;
}

std::string __toStringWithPadding(size_t vNumber, size_t vTotalLength)
{
    std::string Res = std::to_string(vNumber);
    while (Res.size() < vTotalLength)
        Res = " " + Res;
    return Res;
}

std::string GlslShaderErrorParser::parseAndFormat(const std::string& vErrorStr)
{
    auto ErrorSet = parse(vErrorStr);
    if (ErrorSet.empty())
        return "No error when compiling shader";

    std::string ErrorStr = "Found " + std::to_string(ErrorSet.size()) + " errors.\n";
    size_t LineNumLength = 1;
    for (const auto& Error : ErrorSet)
    {
        LineNumLength = std::max(LineNumLength, __getDigitNum(Error.LineNum));
    }

    for (const auto& Error : ErrorSet)
    {
        ErrorStr += "Line " + __toStringWithPadding(Error.LineNum, LineNumLength) + ": ";
        if (!Error.ErrorCode.empty())
            ErrorStr += "'" + Error.ErrorCode + "' - ";
        ErrorStr += Error.ErrorMsg + "\n";
    }

    return ErrorStr;
}
