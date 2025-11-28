#include "../../include/session/Session.h"

#include "../../include/session/SessionManager.h"

namespace http
{
namespace session
{

    Session::Session(const std::string &sessionId, SessionManager *sessionManager, int maxAge)
        : sessionId_(sessionId)
        , maxAge_(maxAge)
        , sessionManager_(sessionManager)
        {
            refresh(); // 初始化时设置过期时间
        }

    // 检查会话是否已过期
    bool Session::isExpired() const
    {
        return std::chrono::system_clock::now() > expiryTime_;
    }

    // 刷新会话的过期时间
    void Session::refresh()
    {
        expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
    }

    // 设置会话数据
    void Session::setValue(const std::string &key, const std::string &value)
    {
        data_[key] = value;
        // 如果设置了manager，自动保存更改
        if(sessionManager_)
        {
            sessionManager_->updateSession(shared_from_this());
        }
    }

    // 获取会话数据
    std::string Session::getValue(const std::string &key) const
    {
        auto it = data_.find(key);
        return it != data_.end() ? it->second : std::string();
    }

    // 删除会话数据
    void Session::remove(const std::string &key)
    {
        data_.erase(key);
    }

    // 清空会话数据
    void Session::clear()
    {
        data_.clear();
    }

} // namespace session
} // namespace http