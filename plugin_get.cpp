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
    char buf_offset[length_of_calculate] = "";
    char buf_hash_result[65] = "";
    bfs::path path_offset;
    path_offset = "./L1";
    path_offset /= file_hash;
    // 判断是否存在完整的文件
    if(exists(path_offset)) {
        std::cout << "is exists\n";
        // 存在 偏移从完整文件里读取
        bfs::fstream file_whole;
        file_whole.open(path_offset, std::ios::binary | std::ios::in);
        assert(file_whole.is_open());
        file_whole.seekg(offset_of_file, std::ios::beg);
        file_whole.read(buf_offset, length_of_calculate);   // 从指定的偏移位置读取指定的长度
        if(file_whole.gcount() != length_of_calculate){
            std::cout << "The file is not enough!" << std::endl;
            return;
        }
        file_whole.close(); // 关闭文件
    } else {
        std::cout << "is not exists!\n";
        // 不存在 偏移从块文件里读取
        bfs::path path_json;
        int length_of_remain = length_of_calculate;
        int size_of_readed = 0;

        // 由于分块了，所以需要偏移量的那个块的偏移量就是直接去除前面块的整就行
        int offset_current_block = offset_of_file % BLOCK_SIZE;

        // 拼接出对应json的路径
        path_json = "./L0";
        path_json /= file_hash;
        path_json.replace_extension("json");

        path_offset = "./L2";

        // 解析json
        std::stringstream buf_json_file;
        bfs::fstream file_json;
        file_json.open(path_json, std::ios::in);
        assert(file_json.is_open());
        buf_json_file << file_json.rdbuf();
        file_json.close(); // 关闭文件
        json_content = buf_json_file.str();
        root_reader.parse(json_content, node);  // 解析json并交给node
        Json::Value json_array = node["block"]; // 取出块文件的信息

        int i = 0;
        for(; static_cast<unsigned int>(i) < node.size(); i++) {
            if(((i + 1) * BLOCK_SIZE) <= offset_of_file) {
                std::cout << "continue" << std::endl;
                continue;
            } else {
                bfs::path path_block_hash = path_offset;    // 为后续拼出块的路径做准备
                string s_block_hash = json_array[i]["value"].asString();    // 拿到当前块对应的hash值

                // hash转路径，并拼接出当前块的完整的路径
                char buf_hash_to_path[80] = "";
                Tools::sha_to_path(const_cast<char *>(s_block_hash.c_str()), buf_hash_to_path);
                path_block_hash /= buf_hash_to_path;
                path_block_hash /= s_block_hash.c_str();

                // 打开块文件并读取偏移后指定的长度
                bfs::fstream file_block;    // 定义
                file_block.open(path_block_hash, std::ios::binary | std::ios::in);  // 打开文件
                assert(file_block.is_open());
                file_block.seekg(offset_current_block, std::ios::beg);  // 偏移位置
                file_block.read(buf_offset + size_of_readed, length_of_remain); //读取数据，就是填充完我们一开始申请的内存块
                size_of_readed = size_of_readed + file_block.gcount();  // 当前总共读取了多少数据
                length_of_remain = length_of_remain - file_block.gcount();  // 还有多少数据没读取
                std::cout << "read: " << file_block.gcount() << ", total read: " << size_of_readed << " , remain: " << length_of_remain << std::endl;
                std::cout << "offset_current_block is:" << offset_current_block << std::endl;
                file_block.close(); //关闭文件

                // 当前块的偏移的计算是：除了第一次读取是有偏移量的，
                // 后续如果还要继续跨块读取，后续块偏移都是0
                offset_current_block = 0;

                if (length_of_remain == 0) {
                    // 完整的读取应该是刚好剩余为0
                    break;
                } else if(length_of_remain < 0) {
                    // 小于0表示出现异常
                    throw 2;
                }
            }
        }
    }
    // 计算hash
    Tools::sha_file_block(buf_offset, buf_hash_result, length_of_calculate);
    std::cout << "want to hash is " << buf_hash_result << std::endl;
    return;
}
void plugin_get::get_block_offset_hash()
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
    file_block.close();
    std::cout << "want to hash is " << string_hash_res << std::endl;
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
        download_block();   // 后续注意下载失败的问题
    }
    buf << json_file.rdbuf();
    json_content = buf.str();   // 把buf里的内容放到string里
    root_reader.parse(json_content, node);  // 解析json并交给node
    json_file.close();  // 记得关闭文件
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