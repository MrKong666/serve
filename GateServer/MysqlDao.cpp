#include "MysqlDao.h"
#include"ConfigMgr.h"
MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"]; 
	const auto& user = cfg["Mysql"]["User"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& port = cfg["Mysql"]["Port"];
	pool_.reset(new MysqlPool(host+":"+port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao()
{
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try
	{
		if (con == nullptr)return false;
		//调用存储过程注册用户
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		//设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);
		stmt->execute();

		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	   // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if(res->next()) {
			int result = res->getInt("result");
			std::cout << "Stored procedure result: " << result << std::endl;
			pool_->returnConnection(std::move(con)); // 归还连接
			return result; // 返回存储过程的结果
		}
		pool_->returnConnection(std::move(con)); // 归还连接
		return -1;
	}
	catch (sql::SQLException&e)
	{
		pool_->returnConnection(std::move(con)); // 归还连接
		std::cout << "Error executing stored procedure: " << e.what() << " SQLState: " << e.getSQLState() <<
			" ErrorCode: " << e.getErrorCode() << std::endl;
		return -1;
	}
}
bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
	auto con = pool_->getConnection();
	try
	{
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}

		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT name FROM user WHERE email=?"));
		//绑定参数
		pstmt->setString(1, email);
		//执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		//遍历结果集，检查邮箱是否匹配
		if(res->next()) {
			std::cout << "CheckEmail query result: " << res->getString("name") << std::endl;
			if (res->getString("name") != name) {
				pool_->returnConnection(std::move(con)); // 归还连接
				return false; // 邮箱不匹配
			}
			pool_->returnConnection(std::move(con)); // 归还连接
			return true; // 邮箱匹配
		}
		pool_->returnConnection(std::move(con)); // 归还连接
		return false; // 没有找到邮箱
	}
	catch (sql::SQLException&e)
	{
		pool_->returnConnection(std::move(con)); // 归还连接
		std::cout << "Error executing query: " << e.what() << " SQLState: " << e.getSQLState() <<
			" ErrorCode: " << e.getErrorCode() << std::endl;
		return false;
	}
}
bool MysqlDao::UpdatePwd(const std::string& email, const std::string& pwd) {
	auto con = pool_->getConnection();
	try
	{
		if(con==nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}
		//准备更新语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd=? WHERE email=?"));
		//绑定参数
		pstmt->setString(1, pwd);
		pstmt->setString(2, email);
		//执行更新
		int updateCount = pstmt->executeUpdate();

		std::cout << "UpdatePwd affected rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con)); // 归还连接
		return true; // 更新成功
	}
	catch (sql::SQLException&e)
	{
		pool_->returnConnection(std::move(con)); // 归还连接
		std::cout << "Error executing update: " << e.what() << " SQLState: " << e.getSQLState() <<
			" ErrorCode: " << e.getErrorCode() << std::endl;
		return false;
	}
}
bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd,   UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con)); // 归还连接
	});
	try
	{
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email=?"));
		//绑定参数
		pstmt->setString(1, email);
		//执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		//遍历结果集，检查密码是否匹配
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			//输出查询
			std::cout << "password in db is " << origin_pwd << std::endl;	
			break;
		}
		if (pwd != origin_pwd)return false;

		userInfo.name = res->getString("name");
		userInfo.email = email;
		userInfo.pwd = origin_pwd;
		userInfo.uid = res->getInt("uid");	
		std::cout << res->getInt("uid")<<"\n";
		return true; // 密码匹配

	}
	catch (sql::SQLException&e)
	{
		std::cout << "Error executing query: " << e.what() << " SQLState: " << e.getSQLState() <<
			" ErrorCode: " << e.getErrorCode() << std::endl;
		return false;
	}
}
