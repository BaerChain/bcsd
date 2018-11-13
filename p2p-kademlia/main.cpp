#include <cereal/archives/binary.hpp>
#include <node/nodeinfo.hpp>
#include <protocol.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/program_options.hpp>
#include <sys/wait.h>
#include "web.h"
using namespace std;

int main(int argc, const char *argv[]) {
	try
	{
		namespace po = boost::program_options;
		po::options_description desc("Options");
		desc.add_options()
			("help,h", "desc ... about the how to strat ...")
			("bootstrap,b", "run initial node")
			("bootstrap_ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address for bootstrap")
			("bootstrap_port", po::value<unsigned short>()->default_value(5001), "Port to send/receive for bootstrap")
			("peer,p", "run peer node")
			("peer_ip", po::value<std::string>(), "IP address for normal node")
			("peer_port", po::value<unsigned short>(), "Port to send/receive for normal node");
	

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 1;
		}

		websp::Web* p_web = new websp::Web();
		std::string addr ;
		unsigned short port ;
		if (vm.count("bootstrap_ip"))
			addr = vm["bootstrap_ip"].as<std::string>();
		if (vm.count("bootstrap_port"))
			port = vm["bootstrap_port"].as<unsigned short>();
		if (vm.count("bootstrap"))
		{
			/* Please set the IP Address and the Port for the Bootstrap node here. */
			//start Bootstrap node
			if (p_web->init_peer_node(addr, port, true))
				cout << "will strating bootstrap node ...." << endl;
			else
				return 1;
		}
		else if(vm.count("peer"))
		{
			//set the IP Address and the Port for the Bootstrap node
			//kdml::NodeInfo initialInfo("127.0.0.1", 5001);
			std::string owner_ip ;
			unsigned short owner_port;
			if (vm.count("peer_ip"))
				owner_ip = vm["peer_ip"].as<std::string>();
			if (vm.count("peer_port"))
				owner_port =vm["peer_port"].as<unsigned short>();
			if (p_web->init_normal_node(addr, port, owner_ip, owner_port))
				cout << "will strating  node ...." << endl;
			else
				return 1;
		}
		else if(vm.empty())
		{
			cout << "there not have any node config so init unsucce ... \n please put --help to check init commend.." << endl;
			return 1;
		}
		
		//¿ªÆô½Úµã
		cout << " strated node ...." << endl;
		//prowork->join();
		p_web->start();
	}
	catch (std::exception& e)
	{
		cout << "catch  exception..." << endl;
		std::cerr << e.what() << std::endl;
		return 1;
	}
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	cout << "peer shout down..." << endl;
	return 0;
}