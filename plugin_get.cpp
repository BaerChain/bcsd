#include <plugin_get.hpp>

void plugin_get::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
           ("get_file", bpo::value<string>(), "get file hash" )
           ("store_path", bpo::value<string>(), "store the final file path")
           ;
}

void plugin_get::plugin_initialize( const variables_map& options )
{
    file_hash = options["get_file"].as<string>();
    store_path = options["store_path"].as<string>();
    r_path = ".";
    std::stringstream buf;
    bfs::path json_path = "./L0";
    json_path /= file_hash;
    json_path.replace_extension("json");
    json_file.open(json_path, std::ios::out);
    buf << json_file.rdbuf();
    json_content = buf.str();
    root_reader.parse(json_content, node);
    json_file.close();
}

void plugin_get::plugin_startup()
{
    std::cout << "starting chain plugin \n";
    
}

void plugin_get::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
    
}