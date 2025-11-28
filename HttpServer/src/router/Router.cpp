#include "../../include/router/Router.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace router
{

void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    RouteKey key{method, path};
    handlers_[key] = std::move(handler);
}

void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
{
    RouteKey key{method, path};
    callbacks_[key] = std::move(callback);
}

bool Router::route(const HttpRequest &req, HttpResponse *resp)
{
    RouteKey key{req.method(), req.path()};

    // 查找处理器
    auto handlerIt = handlers_.find(key);
    if(handlerIt != handlers_.end())
    {
        handlerIt->second->handle(req, resp);
        return true;
    }

    // 查找回调函数
    auto callbackIt = callbacks_.find(key);
    if(callbackIt != callbacks_.end())
    {
        callbackIt->second(req, resp);
        return true;
    }

    // 查找动态路由
    for(const auto &[method, pathRegex, handler] : regexHandlers_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行处理器
        if(method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
            HttpRequest newReq(req);
            extractPathParameters(match, newReq);

            handler->handle(newReq, resp);
            return true;
        }
    }

    // 查找动态路由回调函数
    for(const auto &[method, pathRegex, callback] : regexCallbacks_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行回调函数
        if(method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
            HttpRequest newReq(req);
            extractPathParameters(match, newReq);

            callback(req, resp);
            return true;
        }
    }
    return false;
}

} // namespace router
} // namespace http