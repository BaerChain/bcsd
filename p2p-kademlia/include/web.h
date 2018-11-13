#pragma once
#include <Worker.h>
#include "protocol.hpp"
#include <thread>
#include <chrono>

using namespace wd;
namespace websp
{
	class Web : public Worker
	{
	public:
		Web();
		virtual ~Web();

	public:
		void start();  //
		void stop();

		bool init_peer_node(const std::string& addr, const unsigned short port, bool is_bootstrap = false);
		bool init_normal_node(const std::string& bootstrap_ip, const unsigned short bootstrap_port,
			const std::string& owner_ip, unsigned short owner_port);
		bool can_start() { return is_init_peer; }

	private:
		/// Run network. Not thread-safe; to be called only by worker.
		virtual void doWork();
		virtual void startedWorking();

		/// Shutdown network. Not thread-safe; to be called only by worker.
		virtual void doneWorking();

	private:
		bool m_run;
		kdml::Protocol* pProtocol;
		bool is_init_peer;
	};
}