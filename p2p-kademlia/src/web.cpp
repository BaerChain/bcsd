#include "web.h"

namespace websp
{
	Web::Web() :m_run(false), is_init_peer(false)
	{

	}

	Web::~Web()
	{
		stop();
		//terminate();
	}

	void Web::start()
	{

		if (!can_start())
			return;
		startWorking();
		while (isWorking())
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		// network start failed!
		if (isWorking())
			return;

		std::cout << "Network start failed!" << std::endl;
		doneWorking();
	}

	void Web::stop()
	{
		//停止所有服务
		if (pProtocol)
			delete pProtocol;
		pProtocol = nullptr;

		if (isWorking())
			doneWorking();
	}

	bool Web::init_peer_node(const std::string & addr, const unsigned short port, bool is_bootstrap)
	{
		kdml::NodeInfo nodeinfo(addr, port);
		if (addr.empty() || port <= 0)
		{
			std::cout << "init peer faild ... config is error" << std::endl;
			is_init_peer = false;
			return false;
		}
		if (is_bootstrap)
		{
			//init bootstrap node
			pProtocol = new kdml::Protocol(nodeinfo);
			is_init_peer = true;
			return true;
		}
		else
		{
			//init normal node 
			//TODO rand port and get owner addr
			std::string owner_ip;
			unsigned short owner_port;
			return init_normal_node(addr, port, owner_ip, owner_port);
		}
		is_init_peer = false;
		return false;
	}

	bool websp::Web::init_normal_node(const std::string & bootstrap_ip, const unsigned short bootstrap_port, const std::string & owner_ip, unsigned short owner_port)
	{
		//手动设置 ip port
		if (bootstrap_ip.empty() || bootstrap_port <= 0 || owner_ip.empty() || owner_port <= 0)
		{
			std::cout << "init normal_node faild ... config is error" << std::endl;
			is_init_peer = false;
			return false;
		}
		kdml::NodeInfo bootstrap_info(bootstrap_ip, bootstrap_port);
		kdml::NodeInfo info(owner_ip, owner_port);
		pProtocol = new kdml::Protocol(info);
		//bootstrap_node 发现 可以稍后进行
		pProtocol->bootstrap(bootstrap_info);
		is_init_peer = true;
		return true;
	}

	void Web::doWork()
	{
		try
		{
			if (m_run && pProtocol && !isWorking())
				pProtocol->join();

		}
		catch (std::exception const& _e)
		{
			//cwarn << "Exception in Network Thread: " << _e.what();
			//cwarn << "Network Restart is Recommended.";
		}
	}

	void Web::startedWorking()
	{
		m_run = true;
	}

	void Web::doneWorking()
	{
		//停止服务
		std::cout << "done protocol server .." << std::endl;
		if (pProtocol)
			pProtocol->downwork();
		terminate();
	}
}