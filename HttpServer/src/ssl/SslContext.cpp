#include "../../include/ssl/SslContext.h"
#include <muduo/base/Logging.h>
#include <openssl/err.h>
#include <openssl/pem.h>

namespace ssl
{
SslContext::SslContext(const SslConfig& config)
    : ctx_(nullptr)
    , config_(config)
{

}

SslContext::~SslContext()
{
    if (ctx_)
    {
        SSL_CTX_free(ctx_);
    }
}

bool SslContext::initialize()
{
    // 初始化 OpenSSL
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | 
                    OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);

    // 创建 SSL 上下文
    const SSL_METHOD* method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);
    if (!ctx_)
    {
        handleSslError("Failed to create SSL context");
        return false;
    }

    // 设置 SSL 选项
    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | 
                  SSL_OP_NO_COMPRESSION |
                  SSL_OP_CIPHER_SERVER_PREFERENCE;
    SSL_CTX_set_options(ctx_, options);

    // 加载证书和私钥
    if (!loadCertificates())
    {
        return false;
    }

    // 设置协议版本
    if (!setupProtocol())
    {
        return false;
    }

    // 设置会话缓存
    setupSessionCache();

    LOG_INFO << "SSL context initialized successfully";
    return true;
}

bool SslContext::loadCertificates()
{
    // 加载证书
    if (SSL_CTX_use_certificate_file(ctx_,
     config_.getCertificateFile().c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        handleSslError("Failed to load server certificate");
        return false;
    }

    // 加载私钥
    if (SSL_CTX_use_PrivateKey_file(ctx_, 
        config_.getPrivateKeyFile().c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        handleSslError("Failed to load private key");
        return false;
    }

    // 验证私钥
    if (!SSL_CTX_check_private_key(ctx_))
    {
        handleSslError("Private key does not match the certificate");
        return false;
    }

    // 加载证书链（仅添加中间证书，不替换叶子证书）
    if (!config_.getCertificateChainFile().empty())
    {
        BIO* chainBio = BIO_new_file(config_.getCertificateChainFile().c_str(), "r");
        if (!chainBio)
        {
            handleSslError("Failed to open certificate chain file");
            return false;
        }

        while (true)
        {
            X509* cert = PEM_read_bio_X509(chainBio, nullptr, 0, nullptr);
            if (!cert)
            {
                break; // EOF
            }

            if (SSL_CTX_add_extra_chain_cert(ctx_, cert) != 1)
            {
                X509_free(cert);
                BIO_free(chainBio);
                handleSslError("Failed to add chain certificate");
                return false;
            }
            // 交由 SSL_CTX 管理，无需释放 cert
        }

        BIO_free(chainBio);
    }

    return true;
}

bool SslContext::setupProtocol()
{
    // 设置 SSL/TLS 最低协议版本
    int minVersion = TLS1_2_VERSION;
    switch (config_.getProtocolVersion())
    {
        case SSLVersion::TLS_1_0:
            minVersion = TLS1_VERSION;
            break;
        case SSLVersion::TLS_1_1:
            minVersion = TLS1_1_VERSION;
            break;
        case SSLVersion::TLS_1_2:
            minVersion = TLS1_2_VERSION;
            break;
        case SSLVersion::TLS_1_3:
            minVersion = TLS1_3_VERSION;
            break;
    }

    if (SSL_CTX_set_min_proto_version(ctx_, minVersion) != 1)
    {
        handleSslError("Failed to set minimum protocol version");
        return false;
    }
    
    // TLS 1.2 及以下加密套件
    const char* defaultCipherList = "ECDHE+AESGCM:ECDHE+CHACHA20:!aNULL:!MD5";
    const std::string& configured = config_.getCipherList();
    const char* cipherList = configured.empty() ? defaultCipherList : configured.c_str();
    if (SSL_CTX_set_cipher_list(ctx_, cipherList) <= 0)
    {
        LOG_WARN << "Setting TLS<=1.2 cipher list failed; using OpenSSL defaults";
    }

    // TLS 1.3 加密套件（与 set_cipher_list 分开设置）
    if (SSL_CTX_set_ciphersuites(ctx_, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256") != 1)
    {
        // 部分环境可能禁用或不支持某些套件，降级为默认
        LOG_WARN << "Setting TLS1.3 ciphersuites failed; using OpenSSL defaults";
    }

    // 优先曲线（尽力设置，不作为硬错误）
    if (SSL_CTX_set1_curves_list(ctx_, "X25519:P-256:P-384") != 1)
    {
        LOG_WARN << "Failed to set preferred curves list; continuing";
    }

    return true;
}

void SslContext::setupSessionCache()
{
    SSL_CTX_set_session_cache_mode(ctx_, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(ctx_, config_.getSessionCacheSize());
    SSL_CTX_set_timeout(ctx_, config_.getSessionTimeout());
}

void SslContext::handleSslError(const char* msg)
{
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    LOG_ERROR << msg << ": " << buf;
}

}; // namespace ssl