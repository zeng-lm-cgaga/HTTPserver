#pragma once
#include "SslContext.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/noncopyable.h>
#include <openssl/ssl.h>
#include <memory>

namespace ssl 
{

// 添加消息回调函数类型定义
using MessageCallback = std::function<void(const std::shared_ptr<muduo::net::TcpConnection>&,
                                         muduo::net::Buffer*,
                                         muduo::Timestamp)>;

class SslConnection : muduo::noncopyable 
{
public:
    using TcpConnectionPtr = std::shared_ptr<muduo::net::TcpConnection>;
    using BufferPtr = muduo::net::Buffer*;
    
    SslConnection(const TcpConnectionPtr& conn, SslContext* ctx);
    ~SslConnection();

    void startHandshake();
    void send(const void* data, size_t len);
    void onRead(const TcpConnectionPtr& conn, BufferPtr buf, muduo::Timestamp time);
    bool isHandshakeCompleted() const { return state_ == SSLState::ESTABLISHED; }
    muduo::net::Buffer* getDecryptedBuffer() { return &decryptedBuffer_; }
    // SSL BIO 操作回调
    static int bioWrite(BIO* bio, const char* data, int len);
    static int bioRead(BIO* bio, char* data, int len);
    static long bioCtrl(BIO* bio, int cmd, long num, void* ptr);
    // 设置消息回调函数
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
private:
    void handleHandshake();
    void onEncrypted(const char* data, size_t len);
    void onDecrypted(const char* data, size_t len);
    SSLError getLastError(int ret);
    void handleError(SSLError error);

private:
    SSL*                ssl_; // SSL 连接
    SslContext*         ctx_; // SSL 上下文
    TcpConnectionPtr    conn_; // TCP 连接
    SSLState            state_; // SSL 状态
    BIO*                readBio_;   // 网络数据 -> SSL
    BIO*                writeBio_;  // SSL -> 网络数据
    muduo::net::Buffer  readBuffer_; // 读缓冲区
    muduo::net::Buffer  writeBuffer_; // 写缓冲区
    muduo::net::Buffer  decryptedBuffer_; // 解密后的数据
    MessageCallback     messageCallback_; // 消息回调
};

} // namespace ssl