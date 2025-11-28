#include "../../include/middleware/MiddlewareChain.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace middleware
{
    
    void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware)
    {
        middlewares_.push_back(middleware);
    }

    void MiddlewareChain::processBefore(HttpRequest &request)
    {
        for(auto &middleware : middlewares_)
        {
            middleware->before(request);
        }
    }

    void MiddlewareChain::processAfter(HttpResponse &response)
    {
        try
        {
            // 逆向处理响应，保证中间件的正确执行顺序
            for(auto it = middlewares_.rbegin(); it != middlewares_.rend(); it++)
            {
                if(*it)
                {
                    (*it)->after(response);
                }
            }
        }
        catch(const std::exception &e)
        {
            LOG_ERROR << "ERROR in middleware after processiong" << e.what();
        }
    }

} // namespace  middleware
} // namespace http