#include "../../../include/middleware/cors/CorsMiddleware.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <muduo/base/Logging.h>

namespace http
{
namespace middleware
{
    CorsMiddleware::CorsMiddleware(const CorsConfig &config) : config_(config) {}
    
    void CorsMiddleware::before(HttpRequest &request)
    {
        LOG_DEBUG << "CorsMiddleware::before - Processing request";

        if(request.method() == HttpRequest::Method::mOptions)
        {
            LOG_INFO << "Procession CORS preflight requet"; // preflight 是浏览器发送的 OPTIONS 预检请求
            HttpResponse response;
            handlePreflightRequest(request, response); // 处理预检
            throw response;
        }
    }

    void CorsMiddleware::after(HttpResponse &response)
    {
        LOG_DEBUG << "CorsMiddleware::after - Processiong response";

        if(!config_.allowedOrigins.empty())
        {
            // 直接添加CORS头， 简化处理逻辑
            if(std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end() , "*")
                != config_.allowedOrigins.end() )
            {
                addCorsHeaders(response, "*");
            }
            else
            {
                // 添加第一个允许的源
                addCorsHeaders(response, config_.allowedOrigins[0]);
            }
        }
    }

    bool CorsMiddleware::isOriginAllowed(const std::string &origin) const
    {
        return config_.allowedOrigins.empty() 
             || std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), "*") != config_.allowedOrigins.end()
             || std::find(config_.allowedOrigins.begin(), config_.allowedOrigins.end(), origin) != config_.allowedOrigins.end();
    }

    void CorsMiddleware::handlePreflightRequest(const HttpRequest &request, HttpResponse &response)
    {
        const std::string &origin = request.getHeader("origin");

        if(!isOriginAllowed(origin))
        {
            LOG_WARN << "Origin is not allowed: " << origin;
            response.setStatusCode(HttpResponse::m403Forbidden);
            return;
        }

        addCorsHeaders(response, origin);
        response.setStatusCode(HttpResponse::m204NoContent);
        LOG_INFO << "Preflight request processed successfully";
    }

    void CorsMiddleware::addCorsHeaders(HttpResponse &response, const std::string &origin)
    {
        try
        {
            response.addHeader("Access-Control-Allow-Origin", origin);

            if(config_.allowCredentials)
            {
                response.addHeader("Access-Control-Allow-Credentials", "true");
            }

            if(!config_.allowedMethods.empty())
            {
                response.addHeader("Access-Control-Allow-Methods", join(config_.allowedMethods, ", "));
            }

            if(!config_.allowedHeaders.empty())
            {
                response.addHeader("Access-Control-Allow-Headers", join(config_.allowedHeaders, ", "));
            }
            
            response.addHeader("Access-Control-Max-Age", std::to_string(config_.maxAge));

            LOG_DEBUG << "CORS headers added successfully";
        }
        catch(const std::exception& e)
        {
            LOG_ERROR << "ERROR adding CORS headers: " << e.what();
        }
    }

    // 工具函数：将字符串数组连接成单个字符串
    std::string CorsMiddleware::join(const std::vector<std::string> &strings, const std::string &delimiter)
    {
        std::ostringstream result;
        for( size_t i = 0; i < strings.size(); ++i)
        {
            if(i > 0) result << delimiter;
            result << strings[i];
        }
        return result.str();
    }

} // namespace middleware
} // namespace http