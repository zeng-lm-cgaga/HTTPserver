#pragma once

#include <muduo/net/TcpServer.h>

namespace http
{

class HttpResponse
{
public:
    enum HttpStatusCode
    {
        mUnknown,
        m200Ok = 200,
        m204NoContent = 204,
        m301MovedPermanently = 301,
        m400BadRequest = 400,
        m401Unauthorized = 401,
        m403Forbidden = 403,
        m404NotFound = 404,
        m409Conflict = 409,
        m500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : statusCode_(mUnknown)
        , closeConnection_(close)
    {}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpResponse getStatusCode() const
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