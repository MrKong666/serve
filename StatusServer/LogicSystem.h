#pragma once
#include"const.h"
/*LogicSystem 的函数参数需要 HttpConnection 指针，而 HttpConnection 的业务逻辑又需要调用 LogicSystem。
如果互相 #include 头文件，会导致编译器陷入死循环。
前置声明 (Forward Declaration) 告诉编译器：“有一个类叫这个名字，虽然我现在还没给出细节，
但你可以先通过它的指针或引用来定义接口”。这是解决 C++ 类之间“你中有我，我中有你”耦合问题的标准方案。*/
class HttpConnection;//前置声明解决类之间互引用问题
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() {};
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	void RegGet(std::string, HttpHandler handler);
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	void RegPost(std::string, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler>_post_handlers;
	std::map<std::string, HttpHandler>_get_handlers;
};

