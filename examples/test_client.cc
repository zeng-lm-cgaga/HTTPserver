#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class HttpsClient
{
public:
    HttpsClient()
    {
        // 初始化OpenSsl
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        
        ctx_ = SSL_CTX_new(TLS_client_method());
        if(!ctx_)
        {
            throw std::runtime_error("Failed to create SSL context");
        }
    }

    ~HttpsClient()
    {
        if(ssl_) SSL_free(ssl_);
        if(ctx_) SSL_CTX_free(ctx_);
        if(sock_ > 0 ) close(sock_);  
    }

    void connect(const std::string &host, int port)
    {
        // 创建套接字
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if(sock_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address");
        }

        if(::connect(sock_, (struct sockaddr*) &addr, sizeof(addr)) < 0)
        {
            throw std::runtime_error("Failed to connect");
        }

        ssl_ = SSL_new(ctx_);
        if(!ssl_) {
            throw std::runtime_error("Failed to create SSL");
        }

        SSL_set_fd(ssl_, sock_);
        if(SSL_connect(ssl_) < 0) {
            throw std::runtime_error("Failed to establish SSL connection");
        }

        std::cout << "Connected using: " << SSL_get_cipher(ssl_) << "\n";
    }

    void sendGetRequet(const std::string &path)
    {
        std::cout << "发送请求出去sendGetRequet" << std::endl;
        std::string request = 
            "GET " + path + " HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n";
        if(SSL_write(ssl_, request.c_str(), request.length()) <= 0) 
        {
            throw std::runtime_error("Failed to send request");
        }

        char buffer[4096];
        int bytes;
        while((bytes = SSL_read(ssl_, buffer, sizeof(buffer))) > 0) {
            buffer[bytes] = 0;
            std::cout << buffer;
        }
    }
private:
    SSL_CTX* ctx_ = nullptr;
    SSL* ssl_ = nullptr;
    int sock_ = -1;
};