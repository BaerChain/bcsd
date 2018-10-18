#include <plugin_init.hpp>

void plugin_init::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
            ("root_path", bpo::value<string>(), "the file hash while want to check")
            ;
}

void plugin_init::plugin_initialize( const variables_map& options )
{
    if(options.count("root_path")) {
        root_path = options["root_path"].as<string>();
    } else {
       root_path = ".";
    }
}

void plugin_init::plugin_startup()
{
    make_directories();
    /* 未来应该还有别的事要做
        To do !
        1、是否需要配置文件？
        2、leveldb有没有必要在这里初始化，在别的地方读配置就行？
    */
}

void plugin_init::plugin_shutdown()
{

}

// 创建存储目录结构
int plugin_init::make_directories()
{
    int i = 0;
    bfs::path folder_path = root_path;
    if(!bfs::exists(folder_path)) {
        bfs::create_directory(folder_path);
    }
    for(; i < 3; i++){
        folder_path = root_path;
        string folder_name("L");
        string num_s = boost::lexical_cast<string>(i);
        folder_name.append(num_s);
        folder_path /= folder_name.c_str();
        if(!bfs::exists(folder_path)) {  // 判断目录是否存在
            std::cout << "not exsits " << folder_path << std::endl;
            bfs::create_directory(folder_path);
        }
    }
    return 0;
}