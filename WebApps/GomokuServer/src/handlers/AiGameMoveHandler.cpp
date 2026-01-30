#include "../include/handlers/AiGameMoveHandler.h"

void AiGameMoveHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    LOG_INFO << "AiGameMoveHandler::handle called";
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        std::string isLoggedIn = session->getValue("isLoggedIn");
        
        LOG_INFO << "Check Session: " << session->getId() 
                    << ", isLoggedIn: " << isLoggedIn 
                    << ", userId: " << session->getValue("userId");
        
        
        if (isLoggedIn != "true")
        {
            // 增加详细日志帮助排查
            if (session->getValue("userId").empty()) {
                LOG_WARN << "Unauthorized (Empty Session). ID: " << session->getId();
            } else {
                LOG_WARN << "Unauthorized (Not Logged In). ID: " << session->getId() << ", UID: " << session->getValue("userId");
            }
            
            // 用户未登录，返回未授权错误
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                                 "Unauthorized", true, "application/json", errorBody.size(),
                                 errorBody, resp);
            return;
        }

        int userId = std::stoi(session->getValue("userId"));
        // 解析请求体（仅坐标）
        json request = json::parse(req.getBody());
        int x = request["x"];
        int y = request["y"];
        
        // 整局难度从会话中获取（由 /aiBot/start 决定），不允许每步更改
        std::string difficultyStr = session->getValue("ai_difficulty");
        if (difficultyStr.empty()) difficultyStr = "medium";
        auto parseDifficulty = [](const std::string& d) {
            if (d == "easy") return Difficulty::Easy;
            if (d == "hard") return Difficulty::Hard;
            return Difficulty::Medium;
        };
        Difficulty difficulty = parseDifficulty(difficultyStr);

        // 获取或创建游戏实例
        std::shared_ptr<AiGame> game;
        {
            std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
            if (server_->aiGames_.find(userId) == server_->aiGames_.end())
            {
                server_->aiGames_[userId] = std::make_shared<AiGame>(userId, difficulty);
            }
            game = server_->aiGames_[userId];
        }

        // 不在对局中途更新难度，保持整局一致

        // 处理人类玩家移动
        if (!game->humanMove(x, y))
        {
            json response = {
                {"status", "error"},
                {"message", "Invalid move"}};
            std::string responseBody = response.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(responseBody.size());
            resp->setBody(responseBody);
            return;
        }

        // 检查人类玩家是否获胜
        if (game->isGameOver())
        {
            json response = {
                {"status", "ok"},
                {"board", game->getBoard()},
                {"winner", "human"},
                {"next_turn", "none"}};
            std::string responseBody = response.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(responseBody.size());
            resp->setBody(responseBody);

            {
                std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
                server_->aiGames_.erase(userId); // 这里删掉以后，每次restart都需要重新创建就行
            }
            return;
        }

        // 检查是否平局（在AI移动之前）
        if (game->isDraw())
        {
            LOG_INFO << "Game Draw before AI move";
            json response = {
                {"status", "ok"},
                {"board", game->getBoard()},
                {"winner", "draw"},
                {"next_turn", "none"},
                {"last_move", {{"x", game->getLastMove().first}, {"y", game->getLastMove().second}}}}; // Ensure last_move is present even for Draw
            std::string responseBody = response.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(responseBody.size());
            resp->setBody(responseBody);

            {
                std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
                server_->aiGames_.erase(userId);
            }
            return;
        }

        // AI移动
        LOG_INFO << "Starting AI move for user " << userId;
        game->aiMove();
        LOG_INFO << "AI move completed. Last move: " << game->getLastMove().first << "," << game->getLastMove().second;

        // 检查AI是否获胜
        if (game->isGameOver())
        {
            json response = {
                {"status", "ok"},
                {"board", game->getBoard()},
                {"winner", "ai"},
                {"next_turn", "none"},
                {"last_move", {{"x", game->getLastMove().first}, {"y", game->getLastMove().second}}}};
            std::string responseBody = response.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(responseBody.size());
            resp->setBody(responseBody);

            {
                std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
                server_->aiGames_.erase(userId); // 这里删掉以后，每次restart都需要重新创建就行
            }
            return;
        }

        // 再次检查是否平局（在AI移动之后）
        if (game->isDraw())
        {
            json response = {
                {"status", "ok"},
                {"board", game->getBoard()},
                {"winner", "draw"},
                {"next_turn", "none"},
                {"last_move", {{"x", game->getLastMove().first}, {"y", game->getLastMove().second}}}};
            std::string responseBody = response.dump();

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(responseBody.size());
            resp->setBody(responseBody);

            {
                std::lock_guard<std::mutex> lock(server_->mutexForAiGames_);
                server_->aiGames_.erase(userId); // 这里删掉以后，每次restart都需要重新创建就行
            }
            return;
        }

        // 游戏继续
        json response = {
            {"status", "ok"},
            {"board", game->getBoard()},
            {"winner", "none"},
            {"next_turn", "human"},
            {"last_move", {{"x", game->getLastMove().first}, {"y", game->getLastMove().second}}}};

        std::string responseBody = response.dump();

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(responseBody.size());
        resp->setBody(responseBody);
    }
    catch (const std::exception &e)
    { 
        json response = {
            {"status", "error"},
            {"message", e.what()}};
        std::string responseBody = response.dump();
        server_->packageResp(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error", false, "application/json", responseBody.size(), responseBody, resp);
    }
}