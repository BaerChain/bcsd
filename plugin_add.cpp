#include <plugin_add.hpp>

void plugin_add::set_program_options( options_description& cli, options_description& cfg)
{
    // 这里的两个options_description是框架创建好了传过来的引用，可以直接使用
    // 这里是需要添加参数的位置
    cli.add_options()
            ("add_file", bpo::value<string>(), "the path to the file to be added" )
            ("add_help", "add file into local node")
            ("game_name", bpo::value<string>(), "the name of the game to which the added file belongs")
            ("game_version", bpo::value<string>(), "the version of the game to which the addec file belongs")
            ;
}

void plugin_add::plugin_initialize( const variables_map& options )
{
    // 这里的variables_map也是框架已经给我们存好了的，里面存放了参数的信息，直接用就行
    // 获取需要的文件路径并保存路径
    
    _file_data.file_name = options["add_file"].as<std::string>();
    _file_data.base_file_name = options["game_name"].as<std::string>();
    _file_data.base_file_version = options["game_version"].as<std::string>();
    root_path = ".";
    leveldb_path = "./local";
    config_path = ".";
    leveldb_control.init_db(leveldb_path.string().c_str(), config_path.string().c_str());
    leveldb_control.put_new_file(_file_data);
    std::cout << "file hash is:" << _file_data.file_hash << std::endl;
    //file_path = options["add_file"].as<std::string>();
    //game_name_string = options["game_name"].as<std::string>();
    //game_version_string = options["game_version"].as<std::string>();
    // 获取文件的大小
    //file_size = bfs::file_size(file_path);
    //std::cout << file_size << std::endl;
    // 打开文件，为后续操作准备
    //file_stream.open(file_path, std::ios::in | std::ios::binary);
}

void plugin_add::plugin_startup()
{
    std::cout << "starting chain plugin \n";
    // 计算整个文件的sha256的值并返回到re里
    //char re[65] = "";
    //tools::sha_file(file_stream, re);
    /*
        检测文件是否重复
        :To do! 
    */
    bfs::path copy_file_path = root_path;
    copy_file_path /= "L1";
    copy_file_path /= _file_data.file_hash.c_str();   // 拼接整个文件改名的路径
    // 复制文件到copy_file_path路径里，调用的是boost::filesystem提供的方法
    // 这里逻辑上有个问题：如果人为修改了我们本地的文件，是否校验hash，还是说不管，每次都直接写入，有就覆盖原来的
    if(!bfs::exists(copy_file_path)) {
        bfs::copy_file(_file_data.file_name, copy_file_path);
    } else {
        // 校验hash看是否被篡改
        // To do!
        std::cout << "file is exists!" << std::endl;
    }

    // 确定json文件的位置和名字，并打开json文件
    //bfs::path json_path = "./L0";
    //json_path /= re;
    //json_path.replace_extension("json");
    
    //json_file.open(json_path, std::ios::out);
    //assert(json_file.is_open());

    // 写入原文件的绝对路径，文件名，大小，所属游戏名，所属游戏版本
    //node["path"] = bfs::system_complete(file_path).string();
    //node["name"] = file_path.filename().string();
    //node["size"] = file_size;
    //node["hash"] = re;
    //node["game_name"] = game_name_string;
    //node["game_version"] = game_version_string;

    // 文件分块存入本地，文件名为sha256的值，并存入拿sha256作为分段路径的路径下
    cut_block();
}

void plugin_add::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
    string res = write_to_file.write(node); // 把json对象写入到一个string里
    string db_res;
    //CFirstLevelDb level_db;
    string db_name = "./local";
    string key_string = file_path.filename().string();
    //key_string.append("_1.0");
    //level_db.init_db(db_name);
    //level_db.put_new_file(res);
    std::cout << "json res is:" << res << std::endl;
    std::cout << "key is: " << key_string << std::endl;
    //level_db.get_message(key_string, db_res);
    std::cout << "json res is:" << db_res << std::endl;
    json_file << res;   //写入到文件里
    json_file.close();
    file_stream.close();
}

/***
 * 确定我们程序存储的根目录
 * @return < 0 说明执行失败
 */
int plugin_add::root_dir()
{
    
    return 0;
}

/***
 * 把文件拆分成1M大小的分块，并把分块写入对应规则的路径下
 * @return < 0 说明执行失败，成功时返回值是分块的数量
 */
int plugin_add::cut_block()
{
    file_stream.open(_file_data.file_name, std::ios::binary | std::ios::in);
    // 获取文件的大小
    file_size = bfs::file_size(_file_data.file_name);
    file_stream.clear();
    file_stream.seekp(std::ios::beg);
    bfs::path block_name;
    bfs::fstream block_f;
    int i = 0;
    int left_file_size = this->file_size;
    char buf[BLOCK_SIZE] = "";
    while(1) {
        memset(buf, 0, BLOCK_SIZE * sizeof(char));
        // 拼接出本次文件的名字
        string _block_name("block_");
        string num_s = boost::lexical_cast<string>(i);
        _block_name.append(num_s);
        std::cout << "block_name is " << _block_name << std::endl;
        
        // 判断当前还能读取的分块大小，当为最后一次时，剩余的数量是不大于我们指定的分块大小的
        int read_buf_size = (left_file_size >= BLOCK_SIZE ? BLOCK_SIZE : left_file_size);
        assert(file_stream.read(buf, read_buf_size));

        char res_hash[65] = "";
        tools::sha_file_block(buf, res_hash, read_buf_size);
        std::cout << _block_name << " hash is :" << res_hash << std::endl;
        char cur_path[80] = "";
        tools::sha_to_path(res_hash, cur_path);
        // 把文件块按序号写入json
        Json::Value item;
        item["num"] = i;
        item["value"] = res_hash;
        block.append(item);
        //block[num_s] = res_hash;

        // 把名字赋值给boost库的path类来管理
        block_name = root_path;
        block_name /= "L2";
        block_name /= cur_path;
        block_name /= res_hash;

        block_f.open(block_name, std::ios::out | std::ios::binary);
        assert(block_f.is_open());
        block_f.write(buf, read_buf_size);
        block_f.close(); // 记得要关闭文件
        left_file_size = left_file_size - BLOCK_SIZE; // 减去已经读了的分块大小就是剩余总的大小
        if(left_file_size <= 0) break;
        i++;
    }
    //node["block"] = block;
    string block_res = write_to_file.write(block);
    std::cout << "block res is: " << block_res << std::endl;
    return i + 1;
}
