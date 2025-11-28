#include "../../include/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

namespace http
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    bool ok = true; // 解析每行请求格式是否正确标志
    bool hasMore = true;
    while(hasMore)
    {
        if(state_ == mExpectRequestLine) // 解析首行
        {
            const char *crlf = buf->findCRLF();
            if(crlf)
            {
                ok = processRequestLine(buf->peek(), crlf);
                if(ok)
                {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);
                    state_ = mExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == mExpectHeaders)
        {
            const char *crlf = buf->findCRLF();
            if(crlf)
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if(colon < crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else if(buf->peek() == crlf)
                {
                    // 空行， 结束Header
                    // 根据请求方法和Content-Length判断是否需要继续读取body
                    if(request_.method() == HttpRequest::mPost ||
                       request_.method() == HttpRequest::mPut)
                    {
                        std::string contentLength = request_.getHeader("Content-Length");
                        if(!contentLength.empty())
                        {
                            request_.setContentLength(std::stoi(contentLength));
                            if(request_.contentlength() > 0)
                            {
                                state_ = mExpectBody;
                            }
                            else
                            {
                                state_ = mGotAll;
                                hasMore = false;
                            }
                        }
                        else
                        {
                            // POST/PUT 请求没有 Content-Length， 是HTTP语法错误
                            ok = false;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 等方式直接完成（没有请求体）
                        state_ = mGotAll;
                        hasMore = false;
                    }
                }
                else
                {
                    ok = false; // Header行格式错误
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2); // 开始读取读指针指向的下一行数据
            }
            else
            {
                hasMore = false;
            }
        }
        else if(state_ == mExpectBody)
        {
            // 检查缓冲区是否有足够的数据
            if(buf->readableBytes() < request_.contentlength())
            {
                hasMore = false; // 数据不完整，等待更多数据
                return true;
            }

            // 只读取 Content-Length 指定的长度
            std::string body(buf->peek(), buf->peek() + request_.contentlength());
            request_.setBody(body);

            // 准确移动读指针
            buf->retrieve(request_.contentlength());

            state_ = mGotAll;
            hasMore = false;
        }
    }
    return ok; // ok为false代表语法解析错误
}

bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if(space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if(space != end)
        {
            const char *argumentStart = std::find(start, space, '?'); // 判断请求是否带参数
            if(argumentStart != space) // 带参
            {
                request_.setPath(start, argumentStart); // [start, argumentStart)
                request_.setQueryParameters(argumentStart + 1, space);
            }
            else // 不带参
            {
                request_.setPath(start, space);
            }

            start = space + 1;
            succeed = ((end - start == 8 ) && std::equal(start, end - 1, "HTTP/1."));
            if(succeed)
            {
                if(*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if(*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }   
    }
    return succeed;
}

}

