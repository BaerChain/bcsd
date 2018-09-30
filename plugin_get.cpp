#include <plugin_get.hpp>

void plugin_get::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
            ("get_file", bpo::value<string>(), "get file hash" )
            ("store_path", bpo::value<string>(), "store the final file path")
            ("offset", bpo::value<int>(), "offset num of begin")
            ("length", bpo::value<int>(), "length of calculate")
            ;
}

void plugin_get::plugin_initialize( const variables_map& options )
{
    // 获取参数
    if(options.count("get_file")) {
        file_hash = options["get_file"].as<string>();
    }
    if(options.count("store_path")) {
        store_path = options["store_path"].as<string>();
    }
    if(options.count("offset")) {
        offset_of_file = options["offset"].as<int>();
        length_of_calculate = options["length"].as<int>();
    }
    
    
    std::cout << "hash is:" << file_hash << "\nstore path is:" << store_path << std::endl;
    std::cout << "offset_of_file:" << offset_of_file << "\nlength_of_calculate is:" << length_of_calculate << std::endl;
    r_path = ".";
}   

void plugin_get::plugin_startup()
{
    std::cout << "starting chain plugin \n";
    if(length_of_calculate > 0) {
        get_offset_hash();
    } else {
        get_file();
    }
}

void plugin_get::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
}

// 只是简单测试一下计算偏移后的数据的sha256，后面校验肯定不在这里，也不是这种方式
void plugin_get::get_offset_hash()
{
    bfs::path path_block;
    path_block = "./L2";
    char path_hash[80] = "";
    Tools::sha_to_path(const_cast<char *>(file_hash.c_str()), path_hash);
    path_block /= path_hash;
    path_block /= file_hash.c_str();
    std::cout << "block path is:" << path_block << std::endl;
    bfs::fstream file_block;
    file_block.open(path_block, std::ios::binary | std::ios::in);
    assert(file_block.is_open());
    char string_hash_res[65] = "";
    Tools::offset_to_hash(file_block, offset_of_file, length_of_calculate, string_hash_res);
    std::cout << "block segment hash is:" << string_hash_res << std::endl;
    return;
}
void plugin_get::get_file()
{
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
    return;
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
        // 获取到块数据的数组
        Json::Value block_array = node["block"];
        bfs::fstream target_file;
        // 打开目标文件，准备后续的写入
        target_file.open(store_file_path, std::ios::binary | std::ios::out | std::ios::trunc);
        assert(target_file.is_open());
        
        // 依次把每个块写入到目标文件
        for(; i < block_array.size(); i++) {

            // 按照顺序获取每个块的hash值
            string block_hash = block_array[i]["value"].asString();
            
            // hash转路径
            char block_hash_path[80] = "";
            Tools::sha_to_path(const_cast<char *>(block_hash.c_str()), block_hash_path);
            // 拼出块的路径
            block_path = "./L2";
            block_path /= block_hash_path;
            block_path /= block_hash;
            bfs::fstream block_file;

            // 打开块文件
            block_file.open(block_path, std::ios::in | std::ios::binary);
            assert(block_file.is_open());
            // 获取文件的hash比对存储的hash是否一致，不一致说明json或文件被篡改
            char block_hash_res[65] = "";
            Tools::sha_file(block_file, block_hash_res);
            if(block_hash.compare(block_hash_res) == 0) {
                // 把当前块文件写入到目标文件，没重置过文件指针，会是追加的方式写入
                Tools::file_to_file(target_file, block_file);
            } else {
                throw 1;
            }
            block_file.close(); // 记得关闭块文件
        }
        target_file.close();
    }
    return 0;
}