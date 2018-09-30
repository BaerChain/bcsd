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

namespace Tools {

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