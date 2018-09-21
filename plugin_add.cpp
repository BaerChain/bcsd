#include <plugin_add.hpp>

void plugin_add::set_program_options( options_description& cli, options_description& cfg)
{
    // 这里的两个options_description是框架创建好了传过来的引用，可以直接使用
    // 这里是需要添加参数的位置
    cli.add_options()
           ("add_file", bpo::value<string>(), "想要加入的文件的文件路径" )
           ("add_help", "add file into local node")
           ;
}

void plugin_add::plugin_initialize( const variables_map& options )
{
    // 这里的variables_map也是框架已经给我们存好了的，里面存放了参数的信息，直接用就行
    // 获取需要的文件路径并保存路径
    this->file_path = options["add_file"].as<std::string>();
    std::cout << this->file_path << std::endl;
    // 获取文件的大小
    this->file_size = bfs::file_size(this->file_path);
    std::cout << this->file_size << std::endl;
    // 打开文件，为后续操作准备
    file_stream.open(this->file_path, std::ios::in | std::ios::binary);
    cut_block();
}

void plugin_add::plugin_startup()
{
    std::cout << "starting chain plugin \n";
}

void plugin_add::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
}

int plugin_add::cut_block()
{
    
    return 0;
}