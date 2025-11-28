#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace http
{

namespace session
{

class SessionManager;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(const std::string &sessionId, SessionManager *sessionManager, int maxAge = 360); // 默认一个小时过期

    const std::string& getId() const
    { return sessionId_; }

    bool isExpired() const;
    void refresh(); // 刷新过期时间

    void setManager(SessionManager* sessionManager)
    { sessionManager_ = sessionManager; }

    SessionManager* getManager() const
    { return sessionManager_; }

    // 读取数据
    void setValue(const std::string &key, const std::string &value);
    std::string getValue(const std::string &key) const;
    void remove(const std::string &key);
    void clear();
private:
    std::string                                  sessionId_; // 会话id（唯一）
    std::unordered_map<std::string, std::string> data_; // 会话数据
    std::chrono::system_clock::time_point        expiryTime_; // 将要过期的时间
    int                                          maxAge_; // 过期时间（秒）
    SessionManager*                              sessionManager_;
};

} // namespace  session
} // namespace  http