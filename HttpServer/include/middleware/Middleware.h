#pragma once

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http
{

namespace middleware
{

class Middleware
{
public:
    virtual ~Middleware() = default;

    // 请求前处理
    virtual void before(HttpRequest &req) = 0;

    // 响应后处理
    virtual void after(HttpResponse &resp) = 0;

    // 设置下一个中间件
    void setNext(std::shared_ptr<Middleware> next)
    {
        nextMiddleware_ = next;
    }

private:
    std::shared_ptr<Middleware> nextMiddleware_;
};

} // namespace middleware
} // namespace http