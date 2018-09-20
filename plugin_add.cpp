#include <plugin_add.hpp>

void plugin_add::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
           ("file_path", bpo::value<string>(), "想要加入的文件的文件路径" )
           ("help", "add file into local node")
           ;
}

void plugin_add::plugin_initialize( const variables_map& options )
{
    std::cout << "initialize chain plugin\n";
}

void plugin_add::plugin_startup()
{
    std::cout << "starting chain plugin \n";
}

void plugin_add::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
}