#pragma once

#include <vector>
#include <memory>
#include "Middleware.h"

namespace http
{

namespace middleware
{

class MiddlewareChain
{
public:
    void addMiddleware(std::shared_ptr<Middleware> middleware);
    void processBefore(HttpRequest &request);
    void processAfter(HttpResponse &response);

private:
    std::vector<std::shared_ptr<Middleware>> middlewares_;
};

} // namespace middleware
} // namesppce http