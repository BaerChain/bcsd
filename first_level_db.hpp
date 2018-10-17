#pragma once
#include<iostream>
#include <jsoncpp/json/json.h>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "Tools.hpp"

using namespace std;
//using namespace Tools;

/*
    提供leveldb存储查询服务
	存储传入文件 
    建立需要的索引
    提供对外的增删改接口
*/
typedef Tools::SFileData SFileTransDate;

//level kv  设计结构
struct SLevelSaveKV
{
    vector<string> key_str;    // file name 
    string value;           // file value_name 泛型
    string spli_flag;       // 分隔符
    int is_repeat;          // vaue 时候重复
    int kv_id;              // 每个kv的id
};

class CFirstLevelDb
{
public:
    CFirstLevelDb();
    ~CFirstLevelDb();
    int init_db(const char* name_db, const char* config_name);
    int close_db();
private:

    int load_config(const char* file_name);

    Tools::ESaveErrorCode put_new_kvs(const Tools::SFileData& file_data);
    
public: 
    int delete_file(const std::string& key_string);

    Tools::ESaveErrorCode put_new_file(Tools::SFileData& file_data);

    Tools::ESaveErrorCode update_file(const Tools::SFileData* file_data);

    int get_message(const std::string& key_string, std::string& str_date);

private:
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::Status status;
    boost::filesystem::fstream file_stream;
    Json::FastWriter fwrite;
    Json::Reader reader;
    vector<SLevelSaveKV> v_save_kv;
};

