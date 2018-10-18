#pragma once

#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <jsoncpp/json/json.h>
#include <openssl/sha.h>

#define BLOCK_SIZE (1024 * 1024)

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

using bpo::options_description;
using bpo::variables_map;
using std::string;
using std::vector;

namespace tools {

    struct SFileData 
    {
        string file_name;           // 添加文件的路劲
        string file_version;        // 添加文件的版本
        string base_file_name;      // 游戏的名称
        string base_file_version;   // 游戏的版本
        string file_value;          // 生成的json串
        string file_hash;           // 整个文件的hash
        string file_path;           // 留着扩展

        string get_value(const string& str) const;
    };

    enum ESaveErrorCode
    {
        e_no_error = 0,
        e_file_not_exist,    //文件不存在
        e_file_open_error, 
        e_file_exit,         //文件之前已经存在
        e_key_error,         //存储过程 获取key失败
        e_value_error,       //存储过程 获取value失败
        e_leveldb_save_error,//数据库存储错误
    };

    // 计算文件的sha256的值
    int sha_file(bfs::fstream& fp, char res[]);

    // 计算传入buf的sha256的值
    int sha_file_block(char buf[], char res[], int buf_size);

    // sha256的值转成路径
    int sha_to_path(char sha_val[], char res[]);

    // 把后者文件的内容写入前者
    int file_to_file(bfs::fstream& file_wirte, bfs::fstream& file_read);

    // 偏移量后的指定长度计算sha256的值
    int offset_to_hash(bfs::fstream& file_check, int offset_num, int length_num, char res[]);
}