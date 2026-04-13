#include "CServer.h"
#include"HttpConnection.h"
#include"AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port):_ioc(ioc),
_acceptor(ioc,tcp::endpoint(tcp::v4(),port)) 
{

}



void CServer::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection>new_con = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_con->GetSocket(), [self,new_con](beast::error_code ec) {
		try
		{
			//出错就放弃这个链接，继续监听其他链接
			if (ec) {
				self->Start();
				return;
			}
			//创建新链接，并且创建httpconnection类管理这个链接
			/*std::make_shared<HttpConnection>(std::move(self->_socket))->Start();*/
			new_con->Start();
			self->Start();
		}
		catch ( std::exception& exp)
		{

		}

		});
}
/*痛点：异步操作（如 async_accept 或 async_read）会在未来的某个时间点触发回调。
如果此时原始的 CServer 或 HttpConnection 对象因为超出作用域被销毁了，回调函数访问 this 就会导致程序崩溃。
解决方案：通过继承 std::enable_shared_from_this 并捕获 self 到 Lambda 闭包中，人为地增加了对象的引用计数。
效果：只要异步操作没有完成，闭包就一直持有 self，对象就绝对不会析构。这保证了异步回调的安全性。*/
/*非阻塞循环：这不是传统的 while(true) 阻塞循环，而是通过在回调函数的末尾再次调用 Start() 实现的“逻辑递归”。
如果某个连接接入失败，服务器不会挂掉，而是优雅地放弃当前 socket 并立即启动下一次监听。*/