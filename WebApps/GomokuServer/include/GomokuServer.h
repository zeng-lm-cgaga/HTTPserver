#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <mutex>


#include "AiGame.h"
#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"


class LoginHandler;
class EntryHandler;
class RegisterHandler;
class MenuHandler;
class AiGameStartHandler;
class LogoutHandler;
class AiGameMoveHandler;
class GameBackendHandler;

#define DURING_GAME 1 
#define GAME_OVER 2

#define MAX_AIBOT_NUM 4096

class GomokuServer
{
public:
    GomokuServer(int port,
                 const std::string& name,
                 muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

    void setThreadNum(int numThreads);
    void start();
private:
    void initialize();
    void initializeSession();
    void initializeRouter();
    void initializeMiddleware();
    
    void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
    {
        httpServer_.setSessionManager(std::move(manager));
    }

    http::session::SessionManager*  getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }
    
    void restartChessGameVsAi(const http::HttpRequest& req, http::HttpResponse* resp);
    void getBackendData(const http::HttpRequest& req, http::HttpResponse* resp);

    void packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
                     const std::string& statusMsg, bool close, const std::string& contentType,
                     int contentLen, const std::string& body, http::HttpResponse* resp);

    // 获取历史最高在线人数
    int getMaxOnline() const
    {
        return maxOnline_.load();
    }

    // 获取当前在线人数
    int getCurOnline() const
    {
        return onlineUsers_.size();
    }

    void updateMaxOnline(int online)
    {
        maxOnline_ = std::max(maxOnline_.load(), online);
    }

    // 获取用户总数
    int getUserCount()
    {
        std::string sql = "SELECT COUNT(*) as count FROM users";

        sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
        if (res->next())
        {
            return res->getInt("count");
        }
        return 0;
    }
    
private:
    friend class EntryHandler;
    friend class LoginHandler;
    friend class RegisterHandler;
    friend class MenuHandler;
    friend class AiGameStartHandler;
    friend class LogoutHandler;
    friend class AiGameMoveHandler;
    friend class GameBackendHandler;

private:
    enum GameType
    {
        NO_GAME = 0,
        MAN_VS_AI = 1,
        MAN_VS_MAN = 2
    };
    // 实际业务制定由GomokuServer来完成
    // 需要留意httpServer_提供哪些接口供使用
    http::HttpServer                                 httpServer_;
    http::MysqlUtil                                  mysqlUtil_;
    // userId -> AiBot
    std::unordered_map<int, std::shared_ptr<AiGame>> aiGames_;
    std::mutex                                       mutexForAiGames_;
    // userId -> 是否在游戏中
    std::unordered_map<int, bool>                    onlineUsers_;
    std::mutex                                       mutexForOnlineUsers_; 
    // 最高在线人数
    std::atomic<int>                                 maxOnline_;
};