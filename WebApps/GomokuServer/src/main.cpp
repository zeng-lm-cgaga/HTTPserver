#include <string>
#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "GomokuServer.h"

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  
  std::string serverName = "HttpServer";
  int port = 80;
  
  // 参数解析
  int opt;
  const char* str = "p:";
  while ((opt = getopt(argc, argv, str)) != -1)
  {
    switch (opt)
    {
      case 'p':
      {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }
  
  muduo::Logger::setLogLevel(muduo::Logger::WARN);
  GomokuServer server(port, serverName);
  server.setThreadNum(4);
  server.start();
}
