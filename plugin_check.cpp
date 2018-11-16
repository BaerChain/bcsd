#include <plugin_check.hpp>

void plugin_check::set_program_options( options_description& cli, options_description& cfg)
{
    cli.add_options()
            ("check_file", bpo::value<string>(), "the file hash while want to check")
            ("offset", bpo::value<long long>(), "offset num of begin")
            ("length", bpo::value<long long>(), "length of calculate")
            ;
}

void plugin_check::plugin_initialize( const variables_map& options )
{
    root_path = ".";
    // 初始化数据库
    bfs::path leveldb_path = root_path;
    leveldb_path /= "local";
    bfs::path config_path = root_path;
    config_path /= "../kv_config.json";
    leveldb_control.init_db(leveldb_path.string().c_str(), config_path.string().c_str());
    if(options.count("offset")) {
        check_file_hash = options["check_file"].as<string>();
        offset_of_file = options["offset"].as<long long>();
        length_of_calculate = options["length"].as<long long>();
    }
}

void plugin_check::plugin_startup()
{
    get_offset_hash();
}

void plugin_check::plugin_shutdown()
{

}

// 只是简单测试一下计算偏移后的数据的sha256，后面校验肯定不在这里，也不是这种方式
std::string plugin_check::get_offset_hash()
{
    char buf_offset[length_of_calculate] = "";
    char buf_hash_result[65] = "";
    bfs::path path_offset = root_path;
    path_offset /= "L1";
    path_offset /= check_file_hash;
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
            return std::string();
        }
        file_whole.close(); // 关闭文件
    } else {
        std::cout << "is not exists!\n";
        // 不存在 偏移从块文件里读取
        //bfs::path path_json;
        int length_of_remain = length_of_calculate;
        int size_of_readed = 0;

        // 由于分块了，所以需要偏移量的那个块的偏移量就是直接去除前面块的整就行
        int offset_current_block = offset_of_file % BLOCK_SIZE;

        // 拼接出对应json的路径
        //path_json = "./L0";
        //path_json /= file_hash;
        //path_json.replace_extension("json");

        path_offset = root_path;
        path_offset /= "L2";

        // 解析json
        //std::stringstream buf_json_file;
        //bfs::fstream file_json;
        //file_json.open(path_json, std::ios::in);
        //assert(file_json.is_open());
        //buf_json_file << file_json.rdbuf();
        //file_json.close(); // 关闭文件
        //json_content = buf_json_file.str();
        leveldb_control.get_message(check_file_hash.string(), json_content);
        root_reader.parse(json_content, node);  // 解析json并交给node
        Json::Value json_array = node["block"]; // 取出块文件的信息

        int i = 0;
        for(; static_cast<unsigned int>(i) < node.size(); i++) {

            // hash转路径，并拼接出当前块的完整的路径
            char buf_hash_to_path[80] = "";
            bfs::path path_block_hash = path_offset;    // 为后续拼出块的路径做准备
            string s_block_hash = json_array[i]["value"].asString();    // 拿到当前块对应的hash值
            tools::sha_to_path(const_cast<char *>(s_block_hash.c_str()), buf_hash_to_path);
            path_block_hash /= buf_hash_to_path;
            path_block_hash /= s_block_hash.c_str();

            if(((i + 1) * BLOCK_SIZE) <= offset_of_file) {
                assert(bfs::exists(path_block_hash));
                std::cout << "continue" << std::endl;
                continue;
            } else {

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
    tools::sha_file_block(buf_offset, buf_hash_result, length_of_calculate);
    std::cout << "want to hash is " << buf_hash_result << std::endl;
    return std::string(buf_hash_result);
}
void plugin_check::get_block_offset_hash()
{
    bfs::path path_block;
    path_block = "./L2";
    char path_hash[80] = "";
    tools::sha_to_path(const_cast<char *>(check_file_hash.c_str()), path_hash);
    path_block /= path_hash;
    path_block /= check_file_hash.c_str();
    std::cout << "block path is:" << path_block << std::endl;
    bfs::fstream file_block;
    file_block.open(path_block, std::ios::binary | std::ios::in);
    assert(file_block.is_open());
    char string_hash_res[65] = "";
    tools::offset_to_hash(file_block, offset_of_file, length_of_calculate, string_hash_res);
    file_block.close();
    std::cout << "want to hash is " << string_hash_res << std::endl;
    return;
}

void plugin_check::set_optins(const std::string _check, const std::string _offset, const std::string & _length)
{
	check_file_hash = _check;
	offset_of_file = std::stoll(_offset);
	length_of_calculate = std::stoll(_length);
}
