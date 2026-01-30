#include "../include/handlers/MenuHandler.h"

void MenuHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // JSON 解析使用 try catch 捕获异常
    try
    {
        // 检查用户是否已登录
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
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

        // 获取用户信息
        int userId = std::stoi(session->getValue("userId"));
        std::string username = session->getValue("username");

        std::string reqFile("/home/ubuntu/HTTPserver/WebApps/GomokuServer/resource/menu.html");
        FileUtil fileOperater(reqFile);
        if (!fileOperater.isValid())
        {
            LOG_WARN << reqFile << "not exist.";
            fileOperater.resetDefaultFile();
        }

        std::vector<char> buffer(fileOperater.size());
        fileOperater.readFile(buffer); // 读出文件数据
        std::string htmlContent(buffer.data(), buffer.size());

        // 在HTML内容中插入userId
        size_t headEnd = htmlContent.find("</head>");
        if (headEnd != std::string::npos)
        {
            std::string script = "<script>const userId = '" + std::to_string(userId) + "';</script>";
            htmlContent.insert(headEnd, script);
        }

        // server_->packageResp(req.getVersion(), HttpResponse::k200Ok, "OK"
        //             , false, "text/html", htmlContent.size(), htmlContent, resp);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("text/html");
        resp->setContentLength(htmlContent.size());
        resp->setBody(htmlContent);
    }
    catch (const std::exception &e)
    {
        // 捕获异常，返回错误信息
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}
