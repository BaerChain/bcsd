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
    // 获取参数
    file_hash = options["get_file"].as<string>();
    store_path = options["store_path"].as<string>();
    std::cout << "hash is:" << file_hash << "\nstore path is:" << store_path << std::endl;
    r_path = ".";
}

void plugin_get::plugin_startup()
{
    std::cout << "starting chain plugin \n";
    // 读取文件需要这样一个buf来中转到string里
    std::stringstream buf;
    bfs::path json_path = "./L0";
    json_path /= file_hash;
    json_path.replace_extension("json");
    std::cout << "json file is:" << json_path << std::endl;
    json_file.open(json_path, std::ios::in);

    // 如果本地有这个hash对应的json表示此文件在本地有存储,
    // 否则去网络上检索，并下载到本地存储结构中
    if(!json_file.is_open()) {
        // 下载完以后就存在本地了，就和本地操作是一样的了
        download_block();   //后续注意下载失败的问题
    }
    buf << json_file.rdbuf();
    json_content = buf.str();   //把buf里的内容放到string里
    root_reader.parse(json_content, node);  //解析json并交给node
    json_file.close();  //记得关闭文件
    std::cout << "json is:" << json_content << std::endl;
    store_file_into_path();
}

void plugin_get::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
    
}

int plugin_get::download_block()
{
    return 0;
}

int plugin_get::store_file_into_path()
{
    // 拼接出本地存储的整个文件的路径
    bfs::path whole_file_path = "./L1";
    whole_file_path /= file_hash;

    // 拼接出用户获取文件后存储的路径
    store_file_path = store_path;
    string file_real_name = node["name"].asString();
    store_file_path /= file_real_name;

    // 打开本地存储的单个文件
    bfs::fstream p_whole_file;
    p_whole_file.open(whole_file_path, std::ios::in | std::ios::binary);
    
    // 如果打开成功，说明本地有这个单个文件，失败就去分块拼接
    if(p_whole_file.is_open()) {
        char res[65] = "";
        res[64] = '\0';
        // 计算单个文件的hash值，如果和请求的不同，说明本地文件被篡改了
        Tools::sha_file(p_whole_file, res);
        std::cout << "compare result:" << file_hash.compare(res) << std::endl;
        // 相同就直接把文件复制到用户希望的路径下，文件名改回原来的文件名
        if(file_hash.compare(res) == 0) {
            bfs::copy_file(whole_file_path, store_file_path);
        }
        p_whole_file.close();
    } else {    //没有单个文件就去块里去拼
        unsigned int i = 0;
        bfs::path block_path;
        Json::Value block_array = node["block"];
        std::cout << "block num is:" << block_array.size() << std::endl;
        bfs::fstream target_file;
        target_file.open(store_file_path, std::ios::binary | std::ios::out | std::ios::trunc);
        assert(target_file.is_open());
        for(; i < block_array.size(); i++) {
            string block_hash = block_array[i]["value"].asString();
            block_path = "./L2";
            char block_hash_path[80] = "";
            Tools::sha_to_path(const_cast<char *>(block_hash.c_str()), block_hash_path);
            block_hash_path[79] = '\0';
            block_path /= block_hash_path;
            block_path /= block_hash;
            bfs::fstream block_file;
            block_file.open(block_path, std::ios::in | std::ios::binary);
            Tools::file_to_file(target_file, block_file);
            //target_file << block_file;
            block_file.close();
        }
        target_file.close();
    }
    return 0;
}