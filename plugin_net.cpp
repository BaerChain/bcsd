#include <plugin_net.hpp>

void plugin_net::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
                ("run_as", bpo::value<string>(), "want to run as peer or server")
                ("ip_address", bpo::value<string>(), "Server address to connect to")
                ("port", bpo::value<unsigned short>(), "Server port to connect to")
                ;
}

void plugin_net::plugin_initialize( const variables_map& options )
{
    if(options.count("ip_address")){
        peer_or_server = options["run_as"].as<string>();
        ip_address = options["ip_address"].as<string>();
        _port = options["port"].as<unsigned short>();
    }else{
        peer_or_server = options["run_as"].as<string>();
        _port = options["port"].as<unsigned short>();
    }
    // 读取最初的节点信息，以后要是有公网的铁定不动的节点，可以写到程序里，现在暂时写配置文件
}
void plugin_net::plugin_startup()
{
    ba::io_service ios;
    peer peer_local(ios, _port);
    peer_local.store_path = "..";
    peer_local.node_id = peer_or_server;
    boost::thread thread_of_input(boost::bind(get_input, &peer_local));
    ios.run();
}
void plugin_net::plugin_shutdown()
{
    
}