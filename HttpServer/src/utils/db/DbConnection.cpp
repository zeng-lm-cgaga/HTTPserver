#include "../../../include/utils/db/DbConnection.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http
{
namespace db
{

    DbConnection::DbConnection(const std::string &host,
                               const std::string &user,
                               const std::string &password,
                               const std::string &database)
    : host_(host)
    , user_(user)
    , password_(password)
    , database_(database)
    {
        try
        {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            conn_.reset(driver->connect(host_, user_, password_));
            if(conn_)
            {
                conn_->setSchema(database_);

                // 设置连接属性
                conn_->setClientOption("OPT_RECONNECT", "true");
                conn_->setClientOption("OPT_CONNECT_TIMEOUT", "10");
                conn_->setClientOption("multi_statements", "false");

                // 设置字符集
                std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
                stmt->execute("SET NAMES utf8mb4");

                LOG_INFO << "Database connecttion established";
            }
        }
        catch(const sql::SQLException &e)
        {
            LOG_ERROR << "Failed to create database connection: " << e.what();
            throw DbException(e.what());
        }
    }

    DbConnection::~DbConnection()
    {
        try
        {
            cleanup();
        }
        catch(...)
        {
            // 析构函数中不抛出异常
        }
        LOG_INFO << "Database connection closed";
    }

    bool DbConnection::ping()
    {
        try
        {
            // 不使用getStmt，直接创建新的语句
            std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1"));
            return true;
        }
        catch(const sql::SQLException &e)
        {
            LOG_ERROR << "Ping failed: " << e.what();
            return false;
        }
    }

    bool DbConnection::isValid()
    {
        try
        {
            if(!conn_) return false;
            std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
            stmt->execute("SELECT 1");
            return true;
        }
        catch(const sql::SQLException &e)
        {
            return false;
        }
    }

    void DbConnection::reconnect()
    {
        try
        {
            if(conn_) 
            {
                conn_->reconnect();
            }
            else
            {
                sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
                conn_.reset(driver->connect(host_, user_, password_));
                conn_->setSchema(database_);
            }
        }
        catch(const sql::SQLException &e)
        {
            LOG_ERROR << "Reconnect failed: " << e.what();
            throw DbException(e.what());
        }
    }

    void DbConnection::cleanup()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        try
        {
            if(conn_)
            {
                // 确保完成所有事务
                if(!conn_->getAutoCommit())
                {
                    conn_->rollback();
                    conn_->setAutoCommit(true);
                }

                // 清理所有未处理的结果集
                std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
                while(stmt->getMoreResults())
                {
                    auto result = stmt->getResultSet();
                    while(result && result->next())
                    {
                        // 消费所有结果
                    }
                }
            }
        }
        catch(const std::exception &e)
        {
            LOG_WARN << "Error clearning up connection: " << e.what();
            try
            {
                reconnect();
            }
            catch(...)
            {
                // 忽略重连错误
            }
        }
    }

}// namespace db
}// namespace http