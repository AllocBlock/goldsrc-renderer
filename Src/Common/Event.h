#pragma once
#include "Log.h"

#include <string>
#include <map>
#include <functional>
#include <sstream>

using EventName_t = std::string;
using HookId_t = size_t;
using EventId_t = std::string;

template <typename... Args>
class CEventHandler
{
public:
    CEventHandler() = delete;
    CEventHandler(std::string vEventName): m_EventName(vEventName) {}

    using Callback_t = std::function<void(Args...)>;

    HookId_t hook(Callback_t vCallback)
    {
        m_IdCallbackMap[m_CurHookId] = vCallback;
        return m_CurHookId++;
    }

    void unhook(HookId_t vHookId)
    {
        if (m_IdCallbackMap.at(vHookId))
            m_IdCallbackMap.erase(vHookId);
        else
            Log::log("Warning: event already unhooked");
    }

    void unhookAll()
    {
        m_IdCallbackMap.clear();
    }

    void trigger(const Args&... vArgs) const
    {
        // FIXME: does the order matters?
        for (const auto& Pair : m_IdCallbackMap)
            Pair.second(vArgs...);
    }

    EventId_t generateEventId()
    {
        std::ostringstream Address;
        Address << (void const*)this;
        return m_EventName + "_" + Address.str() + "_" + std::to_string(m_CurEventId++);
    }

private:
    std::string m_EventName = "";
    size_t m_CurHookId = 1;
    size_t m_CurEventId = 1;
    std::map<HookId_t, Callback_t> m_IdCallbackMap;
};

template <>
class CEventHandler<void> {};

#define _DEFINE_EVENT_INTERFACE(Name, ...) public: \
HookId_t hook##Name(CEventHandler<__VA_ARGS__>::Callback_t vCallback) \
{\
    return m_##Name##EventHandler.hook(vCallback); \
} \
\
void unhook##Name(HookId_t vHookId) \
{ \
    m_##Name##EventHandler.unhook(vHookId); \
}

#define _DEFINE_EVENT_HANDLER(Name, ...) protected: \
CEventHandler<__VA_ARGS__> m_##Name##EventHandler = CEventHandler<__VA_ARGS__>("Event_" #Name);


#define _DEFINE_EVENT(Name, ...) _DEFINE_EVENT_INTERFACE(Name, __VA_ARGS__) \
    _DEFINE_EVENT_HANDLER(Name, __VA_ARGS__) 

#define _DEFINE_UPDATE_EVENT(Name, ...) _DEFINE_EVENT(Name##Update, __VA_ARGS__)

