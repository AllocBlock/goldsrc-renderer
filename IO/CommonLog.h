#pragma once
#include <functional>
#include <string>

using LogFunc = std::function<void(std::string)>;

void setGlobalLogFunc(LogFunc vLogFunc);
bool globalLog(std::string vText);