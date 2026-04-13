#pragma once
#include"const.h"
#include<thread>
#include<jdbc/mysql_driver.h>
#include<jdbc/mysql_connection.h>
#include<jdbc/cppconn/prepared_statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/exception.h>
#include<jdbc/cppconn/statement.h>
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lastime) :_con(con), _last_oper_time(lastime) {}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;//上一次操作的时间戳，超时就关掉连接
};
class MysqlPool {
public:
    MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
        try {
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* con = driver->connect(url_, user_, pass_);
                con->setSchema(schema_);
				// 获取当前时间戳
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// 将时间戳转换为秒
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con,timestamp));
            }
        }
        catch (sql::SQLException& e) {
            // 处理异常
            std::cout << "mysql pool init failed" <<e.what()<< std::endl;
        }
    }

    

   
	//心跳保活检查
    void checkConnection() {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());
            pool_.pop();
            Defer defer([this, &con]() {
                if (con) {
                    pool_.push(std::move(con));
                }
			});
			if (con && timestamp - con->_last_oper_time < 300) { // 连接未超时，继续使用
                continue;
			}


            try
            {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->execute("SELECT 1");
				con->_last_oper_time = timestamp; // 更新最后操作时间
				std::cout << "execute timer alive query,cur is " << timestamp << std::endl;
            }
            catch (sql::SQLException&e)
            {
				std::cout << "check connection failed, error: " << e.what() << std::endl;
            }
           
        }
    }


    std::unique_ptr<SqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() {
			if (b_stop_) return true;
			return !pool_.empty();
        });

		if (b_stop_) return nullptr;
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

    void returnConnection(std::unique_ptr<SqlConnection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) return;
        pool_.push(std::move(con));
		cond_.notify_one();
	}

    void Close() {
		b_stop_ = true;
		cond_.notify_all();
    }
    ~MysqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
        while (pool_.size()) {
			pool_.pop();
        }
	}
private:
    std::string url_, user_, pass_, schema_;
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> b_stop_;
    std::thread _check_thread_;
};

struct UserInfo {
    std::string name;
	std::string email;
    std::string pwd;
    int uid;
};


class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& email, const std::string& pwd);
    bool CheckPwd(const std::string& email, const std::string& pwd,   UserInfo& userInfo);

private:
    std::unique_ptr<MysqlPool> pool_;
};

