#include "../../include/session/SessionManager.h"
#include <iomanip>
#include <iostream>
#include <sstream>

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

        std::shared_ptr<Session> session;

        if(!sessionId.empty())
        {
            session = storage_->load(sessionId);
        }

        if(!session || session->isExpired())
        {
            sessionId = generateSessionId();
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
        std::stringstream ss;
        std::uniform_real_distribution<> dist(0, 15);

        // 生成32个字符的会话ID（16进制）
        for(int i = 0; i < 32; ++i)
        {
            ss << std::hex << dist(rng_);
        }
        return ss.str();
    }

    void SessionManager::destorySession(const std::string& sessionId)
    {
        storage_->remove(sessionId);
    }

    void SessionManager::cleanExpiredSessions()
    {}

    std::string SessionManager::getSessionIdFromCookie(const HttpRequest& req)
    {
        std::string sessionId;
        std::string cookie = req.getHeader("cookie");

        if(!cookie.empty())
        {
            size_t pos = cookie.find("sessionId=");
            if(pos != std::string::npos)
            {
                pos += 10; 
                size_t end = cookie.find(";", pos);
                if(end != std::string::npos)
                {
                    sessionId = cookie.substr(pos, end - pos);
                }
                else
                {
                    sessionId = cookie.substr(pos);
                }
            }
        }
        return sessionId;
    }

    void SessionManager::setSessionCookie(const std::string &sessionId, HttpResponse* resp)
    {
        std::string cookie = "sessionId=" + sessionId + "; path=/; HttpOnly"; // 设置响应头cookie中会话id
        resp->addHeader("Set-Cookie", cookie);
    }

} // namespace  session
} // namespace http