#pragma once
#include "SslTypes.h"
#include <string>
#include <vector>

namespace ssl 
{

class SslConfig 
{
public:
    SslConfig();
    ~SslConfig() = default;

    // 证书配置
    void setCertificateFile(const std::string& certFile) { certFile_ = certFile; }
    void setPrivateKeyFile(const std::string& keyFile) { keyFile_ = keyFile; }
    void setCertificateChainFile(const std::string& chainFile) { chainFile_ = chainFile; }
    
    // 协议版本和加密套件配置
    void setProtocolVersion(SSLVersion version) { version_ = version; }
    void setCipherList(const std::string& cipherList) { cipherList_ = cipherList; }
    
    // 客户端验证配置
    void setVerifyClient(bool verify) { verifyClient_ = verify; }
    void setVerifyDepth(int depth) { verifyDepth_ = depth; }
    
    // 会话配置
    void setSessionTimeout(int seconds) { sessionTimeout_ = seconds; }
    void setSessionCacheSize(long size) { sessionCacheSize_ = size; }

    // Getters
    const std::string& getCertificateFile() const { return certFile_; }
    const std::string& getPrivateKeyFile() const { return keyFile_; }
    const std::string& getCertificateChainFile() const { return chainFile_; }
    SSLVersion getProtocolVersion() const { return version_; }
    const std::string& getCipherList() const { return cipherList_; }
    bool getVerifyClient() const { return verifyClient_; }
    int getVerifyDepth() const { return verifyDepth_; }
    int getSessionTimeout() const { return sessionTimeout_; }
    long getSessionCacheSize() const { return sessionCacheSize_; }

private:
    std::string certFile_; // 证书文件
    std::string keyFile_; // 私钥文件
    std::string chainFile_; // 证书链文件
    SSLVersion  version_; // 协议版本
    std::string cipherList_; // 加密套件
    bool        verifyClient_; // 是否验证客户端
    int         verifyDepth_; // 验证深度
    int         sessionTimeout_; // 会话超时时间
    long        sessionCacheSize_; // 会话缓存大小
};

} // namespace ssl