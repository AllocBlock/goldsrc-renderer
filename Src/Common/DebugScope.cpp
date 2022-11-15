#include "DebugScope.h"
#include "Log.h"

#include <stack>

std::stack<std::string> gScopeStack;
size_t gPadding = 0;
const int gPaddingSpaceNum = 4;

Debug::Scope::Scope(const std::string& vName)
{
    m_Name = vName;
    gScopeStack.push(vName);
}

Debug::Scope::~Scope()
{
    _ASSERTE(!gScopeStack.empty());
    _ASSERTE(gScopeStack.top() == m_Name);
    gScopeStack.pop();
}

void Debug::log(const std::string& vMessage)
{
    std::string PaddingUnit = "";
    for (int i = 0; i < gPaddingSpaceNum; ++i)
        PaddingUnit += " ";
    std::string Message = vMessage;
    for (size_t i = 0; i < gPadding; ++i)
        Message = PaddingUnit + Message;

    Log::log(Message);
}
