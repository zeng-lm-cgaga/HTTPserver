#include "../../include/ssl/SslConnection.h"
#include <muduo/base/Logging.h>
#include <openssl/err.h>

namespace ssl
{

// 自定义 BIO 方法
static BIO_METHOD* createCustomBioMethod() 
{
    BIO_METHOD* method = BIO_meth_new(BIO_TYPE_MEM, "custom");
    BIO_meth_set_write(method, SslConnection::bioWrite);
    BIO_meth_set_read(method, SslConnection::bioRead);
    BIO_meth_set_ctrl(method, SslConnection::bioCtrl);
    return method;
}

SslConnection::SslConnection(const TcpConnectionPtr& conn, SslContext* ctx)
    : ssl_(nullptr)
    , ctx_(ctx)
    , conn_(conn)
    , state_(SSLState::HANDSHAKE)
    , readBio_(nullptr)
    , writeBio_(nullptr)
    , messageCallback_(nullptr)
{
    // 创建 SSL 对象
    ssl_ = SSL_new(ctx_->getNativeHandle());
    if (!ssl_) {
        LOG_ERROR << "Failed to create SSL object: " << ERR_error_string(ERR_get_error(), nullptr);
        return;
    }

    // 创建 BIO
    readBio_ = BIO_new(BIO_s_mem());
    writeBio_ = BIO_new(BIO_s_mem());
    
    if (!readBio_ || !writeBio_) {
        LOG_ERROR << "Failed to create BIO objects";
        SSL_free(ssl_);
        ssl_ = nullptr;
        return;
    }

    SSL_set_bio(ssl_, readBio_, writeBio_);
    SSL_set_accept_state(ssl_);  // 设置为服务器模式
    
    // 设置 SSL 选项
    SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);
    
    // 设置连接回调
    conn_->setMessageCallback(
        std::bind(&SslConnection::onRead, this, std::placeholders::_1,
                 std::placeholders::_2, std::placeholders::_3));
}

SslConnection::~SslConnection() 
{
    if (ssl_) 
    {
        SSL_free(ssl_);  // 这会同时释放 BIO
    }
}

void SslConnection::startHandshake() 
{
    SSL_set_accept_state(ssl_);
    handleHandshake();
}

void SslConnection::send(const void* data, size_t len) 
{
    if (state_ != SSLState::ESTABLISHED) {
        LOG_ERROR << "Cannot send data before SSL handshake is complete";
        return;
    }
    
    int written = SSL_write(ssl_, data, len);
    if (written <= 0) {
        int err = SSL_get_error(ssl_, written);
        LOG_ERROR << "SSL_write failed: " << ERR_error_string(err, nullptr);
        return;
    }
    
    char buf[4096];
    int pending;
    while ((pending = BIO_pending(writeBio_)) > 0) {
        int bytes = BIO_read(writeBio_, buf, 
                           std::min(pending, static_cast<int>(sizeof(buf))));
        if (bytes > 0) {
            conn_->send(buf, bytes);
        }
    }
}

void SslConnection::onRead(const TcpConnectionPtr& conn, BufferPtr buf, 
                         muduo::Timestamp time) 
{
    if (state_ == SSLState::HANDSHAKE) {
        // 将数据写入 BIO
        BIO_write(readBio_, buf->peek(), buf->readableBytes());
        buf->retrieve(buf->readableBytes());
        handleHandshake();
        return;
    } else if (state_ == SSLState::ESTABLISHED) {
        // 解密数据
        char decryptedData[4096];
        int ret = SSL_read(ssl_, decryptedData, sizeof(decryptedData));
        if (ret > 0) {
            // 创建新的 Buffer 存储解密后的数据
            muduo::net::Buffer decryptedBuffer;
            decryptedBuffer.append(decryptedData, ret);
            
            // 调用上层回调处理解密后的数据
            if (messageCallback_) {
                messageCallback_(conn, &decryptedBuffer, time);
            }
        }
    }
}

void SslConnection::handleHandshake() 
{
    int ret = SSL_do_handshake(ssl_);
    
    if (ret == 1) {
        state_ = SSLState::ESTABLISHED;
        LOG_INFO << "SSL handshake completed successfully";
        LOG_INFO << "Using cipher: " << SSL_get_cipher(ssl_);
        LOG_INFO << "Protocol version: " << SSL_get_version(ssl_);
        
        // 握手完成后，确保设置了正确的回调
        if (!messageCallback_) {
            LOG_WARN << "No message callback set after SSL handshake";
        }
        return;
    }
    
    int err = SSL_get_error(ssl_, ret);
    switch (err) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // 正常的握手过程，需要继续
            break;
            
        default: {
            // 获取详细的错误信息
            char errBuf[256];
            unsigned long errCode = ERR_get_error();
            ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
            LOG_ERROR << "SSL handshake failed: " << errBuf;
            conn_->shutdown();  // 关闭连接
            break;
        }
    }
}

void SslConnection::onEncrypted(const char* data, size_t len) 
{
    writeBuffer_.append(data, len);
    conn_->send(&writeBuffer_);
}

void SslConnection::onDecrypted(const char* data, size_t len) 
{
    decryptedBuffer_.append(data, len);
}

SSLError SslConnection::getLastError(int ret) 
{
    int err = SSL_get_error(ssl_, ret);
    switch (err) 
    {
        case SSL_ERROR_NONE:
            return SSLError::NONE;
        case SSL_ERROR_WANT_READ:
            return SSLError::WANT_READ;
        case SSL_ERROR_WANT_WRITE:
            return SSLError::WANT_WRITE;
        case SSL_ERROR_SYSCALL:
            return SSLError::SYSCALL;
        case SSL_ERROR_SSL:
            return SSLError::SSL;
        default:
            return SSLError::UNKNOWN;
    }
}

void SslConnection::handleError(SSLError error) 
{
    switch (error) 
    {
        case SSLError::WANT_READ:
        case SSLError::WANT_WRITE:
            // 需要等待更多数据或写入缓冲区可用
            break;
        case SSLError::SSL:
        case SSLError::SYSCALL:
        case SSLError::UNKNOWN:
            LOG_ERROR << "SSL error occurred: " << ERR_error_string(ERR_get_error(), nullptr);
            state_ = SSLState::ERROR;
            conn_->shutdown();
            break;
        default:
            break;
    }
}

int SslConnection::bioWrite(BIO* bio, const char* data, int len) 
{
    SslConnection* conn = static_cast<SslConnection*>(BIO_get_data(bio));
    if (!conn) return -1;

    conn->conn_->send(data, len);
    return len;
}

int SslConnection::bioRead(BIO* bio, char* data, int len) 
{
    SslConnection* conn = static_cast<SslConnection*>(BIO_get_data(bio));
    if (!conn) return -1;

    size_t readable = conn->readBuffer_.readableBytes();
    if (readable == 0) 
    {
        return -1;  // 无数据可读
    }

    size_t toRead = std::min(static_cast<size_t>(len), readable);
    memcpy(data, conn->readBuffer_.peek(), toRead);
    conn->readBuffer_.retrieve(toRead);
    return toRead;
}

long SslConnection::bioCtrl(BIO* bio, int cmd, long num, void* ptr) 
{
    switch (cmd) 
    {
        case BIO_CTRL_FLUSH:
            return 1;
        default:
            return 0;
    }
}


} // namespace ssl 