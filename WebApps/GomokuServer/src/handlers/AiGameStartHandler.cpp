#include "../include/handlers/AiGameStartHandler.h"

void AiGameStartHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    auto session = server_->getSessionManager()->getSession(req, resp);
    if (session->getValue("isLoggedIn") != "true")
    {
        // 页面路由未登录，重定向到登录页
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k302Found, "Found");
        resp->addHeader("Location", "/entry");
        resp->setCloseConnection(false);
        resp->setContentType("text/plain");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }

    int userId = std::stoi(session->getValue("userId"));

    // 解析难度（优先取URL参数，其次取会话，默认medium）
    auto diffStr = req.getQueryParameters("difficulty");
    if (diffStr.empty()) {
        diffStr = session->getValue("ai_difficulty");
        if (diffStr.empty()) diffStr = "medium";
    }
    auto parseDifficulty = [](const std::string& d) {
        if (d == "easy") return Difficulty::Easy;
        if (d == "hard") return Difficulty::Hard;
        return Difficulty::Medium;
    };
    Difficulty diff = parseDifficulty(diffStr);
    // 固定整局难度到会话
    session->setValue("ai_difficulty", diffStr);

    // 创建新对局并设置难度
    {
        std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
        if (server_->aiGames_.find(userId) != server_->aiGames_.end())
            server_->aiGames_.erase(userId);
        server_->aiGames_[userId] = std::make_shared<AiGame>(userId, diff);
    }

    // 创建一个ai机器人，它就while不断地执行下棋逻辑
    std::string reqFile("/home/ubuntu/HTTPserver/WebApps/GomokuServer/resource/ChessGameVsAi.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid())
    {
        LOG_WARN << reqFile << "not exist.";
        fileOperater.resetDefaultFile(); // FIXME:其实这里可能不必要，后续删了吧，不过其实也不会调用到毕竟详细地址是我服务端定义的
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer);
    std::string htmlContent(buffer.data(), buffer.size());

    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("text/html");
    resp->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    resp->addHeader("Pragma", "no-cache");
    resp->addHeader("Expires", "0");
    resp->setContentLength(htmlContent.size());
    resp->setBody(htmlContent);
}
