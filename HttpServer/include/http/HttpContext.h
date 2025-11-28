#pragma once

#include <iostream>
#include <muduo/net/TcpServer.h>
#include "HttpRequest.h"

namespace http
{

class HttpContext
{
public:
    enum HttpRequestParseState
    {
        mExpectRequestLine, // 解析请求行
        mExpectHeaders,     // 解析请求头
        mExpectBody,        // 解析请求体
        mGotAll,            // 解析完成
    };

    HttpContext()
    : state_(mExpectRequestLine)
    {}

    bool parseRequest(muduo::net::Buffer* buf, muduo::Timestamp receiveTime);
    bool gotAll() const
    { return state_ == mGotAll; }

    void reset()
    {
        state_ = mExpectRequestLine;
        HttpRequest dummyDate;
        request_.swap(dummyDate);
    }

    const HttpRequest& request() const
    { return request_; }

    HttpRequest& request()
    { return request_; }

private:
    bool processRequestLine(const char* begin, const char* end);
private:
    HttpRequestParseState state_;
    HttpRequest request_;
};
}