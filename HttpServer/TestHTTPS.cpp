#include "../include/http/HttpServer.h"
#include <muduo/base/Logging.h>
#include "../include/ssl/SslConfig.h"

int main ( int argc , char * argv [ ] )
{
// 设置日志级别
muduo :: Logger :: setLogLevel ( muduo :: Logger :: DEBUG ) ;

try
{
// 创建服务器实例
auto serverPtr = std :: make_unique < http :: HttpServer > ( 443 , "HTTPSServer" , true ) ;

// 加载 SSL 配置
ssl :: SslConfig sslConfig ;

// 获取当前工作目录
char cwd [ PATH_MAX ] ;
if ( getcwd ( cwd , sizeof ( cwd ) ) != nullptr )
{
LOG_INFO << "Current working directory: " << cwd ;
}

// 设置证书文件（使用绝对路径）
std :: string certFile = "/etc/letsencrypt/live/growingshark.asia/fullchain.pem" ;
std :: string keyFile = "/etc/letsencrypt/live/growingshark.asia/privkey.pem" ;

LOG_INFO << "Loading certificate from: " << certFile ;
LOG_INFO << "Loading private key from: " << keyFile ;

sslConfig . setCertificateFile ( certFile ) ;
sslConfig . setPrivateKeyFile ( keyFile ) ;
sslConfig . setProtocolVersion ( ssl :: SSLVersion :: TLS_1_2 ) ;

// 验证文件是否存在
if ( access ( certFile . c_str ( ) , R_OK ) != 0 )
{
LOG_FATAL << "Cannot read certificate file: " << certFile ;
return 1 ;
}
if ( access ( keyFile . c_str ( ) , R_OK ) != 0 )
{
LOG_FATAL << "Cannot read private key file: " << keyFile ;
return 1 ;
}

serverPtr -> setSslConfig ( sslConfig ) ;

// 设置线程数
serverPtr -> setThreadNum ( 4 ) ;

// 添加一些测试路由
serverPtr -> Get ( "/" , [ ] ( const http :: HttpRequest & req , http :: HttpResponse * resp )
{
resp -> setStatusCode ( http :: HttpResponse :: k200Ok ) ;
resp -> setStatusMessage ( "OK" ) ;
resp -> setContentType ( "text/plain" ) ;
resp -> setBody ( "Hello, World!" ) ; } ) ;

// 启动服务器
LOG_INFO << "Server starting on port 443..." ;
serverPtr -> start ( ) ;
}
catch ( const std :: exception & e )
{
LOG_FATAL << "Server start failed: " << e . what ( ) ;
return 1 ;
}

return 0 ;
}