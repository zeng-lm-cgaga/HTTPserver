#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <regex>
#include <vector>

#include "RouterHandler.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http
{
namespace router
{
class Router
{
public:
    using HandlerPtr = std::shared_ptr<RouterHandler>;
    using HandlerCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

    // 路由键（请求方法 + URI）
    struct  RouteKey
    {
        HttpRequest::Method method;
        std::string path;

        bool operator==(const RouteKey &other) const
        {
            return method == other.method && path == other.path;
        }
    };
    
    // 为RouteKey 定义哈希函数
    struct RouteKeyHash
    {
        // 组合“方法哈希”和“路径哈希”，减少碰撞（31是经典哈希因子）
        size_t operator()(const RouteKey &key) const
        {
            size_t methodHash = std::hash<int>{}(static_cast<int>(key.method));
            size_t pashHash = std::hash<std::string>{}(key.path);
            return methodHash * 31 + pashHash;
        }
    };

    // 注册路由处理器
    void registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler);
    
    // 注册回调函数形式路由器
    void registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback);

    // 注册动态路由处理器
    void addRegexHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
    {
        std::regex pathRegex = convertToRegex(path);
        regexHandlers_.emplace_back(method, pathRegex, handler);
    }

    // 注册动态路由处理函数
    void addRegexCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
    {
        std::regex pathRegex = convertToRegex(path);
        regexCallbacks_.emplace_back(method, pathRegex, callback);
    }

    bool route(const HttpRequest &req, HttpResponse *resp);

private:
    std::regex convertToRegex(const std::string &pathPattern)
    {
        // 将路径模式转换为正则表达式， 支持匹配任意路径参数
        std::string regexPattern = "^" + std::regex_replace(pathPattern, std::regex(R"(/:([^/]+))"), R"(/([^/]+))") + "$";
        return std::regex(regexPattern);
    }

    // 提取路径参数
    void extractPathParameters(const std::smatch &match, HttpRequest &request)
    {
        for(size_t i = 1; i < match.size(); ++i)
        {
            request.setPathParameters("param" + std::to_string(i), match[i].str());
        }
    }
private:
    struct RouteCallbackObj
    {
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerCallback callback_;
        RouteCallbackObj(HttpRequest::Method method, std::regex pathRegex, const HandlerCallback &callback)
                         : method_(method), pathRegex_(pathRegex), callback_(callback) {}
    };

    struct RouteHandlerObj
    {
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerPtr handler_;
        RouteHandlerObj(HttpRequest::Method method, std::regex pathRegex, const HandlerPtr handler)
                       : method_(method), pathRegex_(pathRegex), handler_(handler) {}
    };

    std::unordered_map<RouteKey, HandlerPtr, RouteKeyHash>      handlers_;
    std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> callbacks_;
    std::vector<RouteHandlerObj>                                regexHandlers_;
    std::vector<RouteCallbackObj>                               regexCallbacks_; 
};

} // namespace router
} // namespace http