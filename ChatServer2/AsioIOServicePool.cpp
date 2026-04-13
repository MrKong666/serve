#include "AsioIOServicePool.h"
#include<iostream>
using namespace std;
AsioIOServicePool::~AsioIOServicePool()
{
	stop();
	std::cout << "AsioIOServicePool destruct\n";
}
boost::asio::io_context& AsioIOServicePool::GetIOService()
{
	auto& service = _ioServices[_nextIOService];
	_nextIOService = (_nextIOService + 1) % _ioServices.size();
	return service;
}
AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),
_works(size),_nextIOService(0)
{
	for (std::size_t i = 0; i < _ioServices.size(); i++) {
		_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i].get_executor()));
	}

	for (std::size_t i = 0; i < _ioServices.size(); i++) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}
void AsioIOServicePool::stop() {
	for (auto& work : _works) {
		work->get_executor().context().stop();
		work->reset();
	}
	for (auto& t : _threads) {
				if(t.joinable())
					t.join();
	}
}
