#include "../../include/session/SessionStorage.h"
#include <iostream>

namespace http
{

namespace session
{

    void MemorySessionStorage::save(std::shared_ptr<Session> session)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[session->getId()] = session;
    }

    // 加载会话
    std::shared_ptr<Session> MemorySessionStorage::load(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if(it != sessions_.end())
        {
            if(!it->second->isExpired())
            {
                return it->second;
            }
            else
            {
                sessions_.erase(it);
            }
        }

        return nullptr;
    }

    void MemorySessionStorage::remove(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(sessionId);
    }

} // namespace session
} // namespace http