#include "../../include/session/SessionManager.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <muduo/base/Logging.h>

namespace http
{
namespace session
{

    // 初始化会话管理器，设置会话存储对象和随机数生成器
    SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
        : storage_(std::move(storage))
        , rng_(std::random_device{}()) // 初始化随机数生成器，用于生成随机的会话ID
    {}

    // 从请求中获取或创建会话
    std::shared_ptr<Session> SessionManager::getSession(const HttpRequest &req, HttpResponse *resp)
    {
        std::string sessionId = getSessionIdFromCookie(req);
        LOG_INFO << "Getting session for cookie ID: " << sessionId << ", Cookie Header: " << req.getHeader("Cookie");

        std::shared_ptr<Session> session;

        if(!sessionId.empty())
        {
            session = storage_->load(sessionId);
             if (session) LOG_INFO << "Session loaded from storage: " << sessionId;
             else LOG_INFO << "Session not found in storage: " << sessionId;
        }

        if(!session || session->isExpired())
        {
             if (session && session->isExpired()) LOG_INFO << "Session expired: " << sessionId;
            sessionId = generateSessionId();
            LOG_INFO << "Generating new session ID: " << sessionId;
            session = std::make_shared<Session>(sessionId, this);
            setSessionCookie(sessionId, resp);
        }
        else
        {
            session->setManager(this); // 为现有会话设置管理器
        }

        session->refresh();
        storage_->save(session); // 这里可能有问题，需要确保正确保存会话
        return session;
    }

    // 生成唯一的会话标识符
    std::string SessionManager::generateSessionId()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        static const char* hex = "0123456789abcdef";
        std::uniform_int_distribution<int> dist(0, 15);
        std::string id;
        id.reserve(32);
        for (int i = 0; i < 32; ++i)
        {
            id.push_back(hex[dist(rng_)]);
        }
        return id;
    }

    void SessionManager::destroySession(const std::string& sessionId)
    {
        storage_->remove(sessionId);
    }

    void SessionManager::cleanExpiredSessions()
    {}

    std::string SessionManager::getSessionIdFromCookie(const HttpRequest& req)
    {
        std::string sessionId;
        std::string cookie = req.getHeader("Cookie");
        
        // Debug Log
        if (!cookie.empty()) {
            LOG_INFO << "Received Cookie Header: [" << cookie << "]";
        } else {
            LOG_INFO << "Received Request with NO Cookie Header";
        }

        // 简单的解析，查找 "sessionId="
        // 改进：确保匹配的是完整键名
        if(!cookie.empty())
        {
            std::string key = "sessionId=";
            size_t pos = cookie.find(key);
            while (pos != std::string::npos)
            {
                // check prefix (start of string or space or semicolon)
                if (pos == 0 || cookie[pos-1] == ' ' || cookie[pos-1] == ';')
                {
                   pos += key.length();
                   size_t end = cookie.find(";", pos);
                   if(end != std::string::npos)
                       sessionId = cookie.substr(pos, end - pos);
                   else
                       sessionId = cookie.substr(pos);
                   break;
                }
                pos = cookie.find(key, pos + 1);
            }
        }
        return sessionId;
    }

    void SessionManager::setSessionCookie(const std::string &sessionId, HttpResponse* resp)
    {
        // 回退到最兼容的设置：SameSite=Lax, HttpOnly. 移除 Secure 以避免本地证书问题
        std::string cookie = "sessionId=" + sessionId + "; path=/; HttpOnly; SameSite=Lax";
        // LOG_INFO << "Setting cookie: " << cookie;
        resp->addHeader("Set-Cookie", cookie);
    }

} // namespace  session
} // namespace http
