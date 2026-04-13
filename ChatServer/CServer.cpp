#include "CServer.h"

#include"AsioIOServicePool.h"
#include"UserMgr.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short port): 
_acceptor(ioc,tcp::endpoint(tcp::v4(),port)) ,_io_context(_io_context)
{
	std::cout << "Server start success,listen on port:" << _port << "\n";
	StartAccept();
}

CServer::~CServer()
{
}



void CServer::HandleAccept(std::shared_ptr<CSession>new_session, 
	const boost::system::error_code& error)
{
	if (!error) {
		new_session->Start();
		std::lock_guard<std::mutex>lock(_mutex);
		_sessions.insert(std::make_pair(
			new_session->GetSessionId(), new_session));
	}
	else {
		std::cout << "session accept failed ,error is"
			<< error.what();

	}
	StartAccept();
}

void CServer::StartAccept()
{
	auto& _io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<CSession>new_session = std::make_shared<CSession>(_io_context, this);
	_acceptor.async_accept(new_session->GetSocket(),
		std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1
	));
}
void CServer::ClearSession(std::string uuid)
{
	if (_sessions.find(uuid) != _sessions.end()) {
		 
		UserMgr::GetInstance()->RmvUserSession(_sessions[uuid]->GetUserId());
	}

	{
		lock_guard<mutex> lock(_mutex);
		_sessions.erase(uuid);
	}
}


 