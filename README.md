# HTTPserver

一个基于 muduo 网络库的高性能 C++ HTTP 服务器项目，展示了现代 C++ 的最佳实践和性能优化技术。

A high-performance C++ HTTP server based on the muduo network library, demonstrating modern C++ best practices and performance optimization techniques.

## 特性 (Features)

- ⚡ 高性能 Reactor 网络模型 (High-performance Reactor network model)
- 🔒 SSL/TLS 支持 (SSL/TLS support)
- 🗄️ 数据库连接池 (Database connection pool)
- 🛣️ 灵活的路由系统 (Flexible routing system)
- 🔧 中间件支持 (Middleware support)
- 💾 会话管理 (Session management)
- 🎯 CORS 跨域支持 (CORS support)

## 📚 C++ 面试准备指南 (Interview Preparation Guides)

本项目包含详细的 C++ 面试准备文档，深入分析项目中使用的 C++ 技术和性能优化：

This project includes comprehensive C++ interview preparation documentation with in-depth analysis of C++ techniques and performance optimizations used:

- **[中文版 (Chinese)](./CPP_INTERVIEW_GUIDE.md)** - C++ 面试准备指南
- **[English Version](./CPP_INTERVIEW_GUIDE_EN.md)** - C++ Interview Preparation Guide

### 涵盖主题 (Topics Covered)

1. 智能指针与内存管理 (Smart Pointers & Memory Management)
2. 数据库连接池设计 (Database Connection Pool Design)
3. 路由系统与哈希优化 (Routing System & Hash Optimization)
4. Reactor 网络模型 (Reactor Network Model)
5. STL 容器选择 (STL Container Selection)
6. 线程安全与并发 (Thread Safety & Concurrency)
7. 设计模式应用 (Design Pattern Applications)
8. RAII 与资源管理 (RAII & Resource Management)
9. 移动语义优化 (Move Semantics Optimization)
10. 性能优化总结 (Performance Optimization Summary)

## 构建 (Build)

```bash
mkdir build
cd build
cmake ..
make
```

## 依赖 (Dependencies)

- muduo 网络库 (muduo network library)
- MySQL C++ Connector
- OpenSSL
- C++17 或更高版本 (C++17 or higher)
