#include "nearimgui.h"

#include <vector>
#include <mutex>

namespace NGui::Detail
{

const char* CacheString(std::string_view sv)
{
    thread_local std::string buf;
    buf.assign(sv);
    return buf.c_str();
}

const char* CacheString(std::string&& s)
{
    thread_local std::string buf;
    buf = s;
    return buf.c_str();
}

std::vector<std::unique_ptr<BaseCallback>> callbacks;
std::mutex callbacksLock;

BaseCallback* CacheCallback(std::unique_ptr<BaseCallback>&& callback)
{
    std::lock_guard guard(callbacksLock);
    callbacks.push_back(std::move(callback));
    return callbacks.back().get();
}

}
namespace NGui
{
    void NewFrame()
    {
        Detail::callbacks.clear();
    }
}
