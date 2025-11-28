#include "../../include/http/HttpResponse.h"

namespace http
{

void HttpResponse::appendToBuffer(muduo::net::Buffer *ouputBuf) const
{
    // 将HttpResponse封装的信息格式化输出
    char buf[32];
    //将状态信息放入可变长缓冲区中
    snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);

    ouputBuf->append(buf);
    ouputBuf->append(statusMessage_);
    ouputBuf->append("\r\n");

    if(closeConnection_)
    {
        ouputBuf->append("Connection: close\r\n");
    }
    else
    {
        ouputBuf->append("Connection: Keep-Alive\r\n");
    }

    for(const auto& header : headers_)
    {
        ouputBuf->append(header.first);
        ouputBuf->append(": ");
        ouputBuf->append(header.second);
        ouputBuf->append("\r\n");
    }
    ouputBuf->append("\r\n");

    ouputBuf->append(body_);
}
 
void HttpResponse::setStatusLine(const std::string &version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

}