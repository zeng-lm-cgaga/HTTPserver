#pragma once

#include <muduo/net/Buffer.h>
#include <map>
#include <string>

namespace http
{

class HttpResponse
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k302Found = 302,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : httpVersion_("HTTP/1.1")
        , statusCode_(k200Ok)
        , statusMessage_("OK")
        , closeConnection_(close)
    {}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    void setContentType(const std::string &contentType)
    { addHeader("Content-Type", contentType); }

    void setContentLength(uint64_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    void addHeader(const std::string &key, const std::string &value)
    { headers_[key] = value; }

    void setBody(const std::string &body)
    { 
        body_ = body;
    }

    void setStatusLine(const std::string& version,
                       HttpStatusCode statusCode,
                       const std::string &statusMessage);

    void setErrorHeader(){}

    void appendToBuffer(muduo::net::Buffer *outputBuff) const;

private:
    std::string                        httpVersion_;
    HttpStatusCode                     statusCode_;
    std::string                        statusMessage_;
    bool                               closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string                        body_;
    bool                               isFile_;
};

}