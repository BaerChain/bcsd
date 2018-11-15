#pragma once
#include<iostream>
#include <jsoncpp/json/json.h>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "tools.hpp"

using namespace std;
//using namespace tools;

/*
    提供leveldb存储查询服务
	存储传入文件 
    建立需要的索引
    提供对外的增删改接口
*/
typedef tools::SFileData SFileTransDate;

//level kv  设计结构
struct SLevelSaveKV
{
    vector<string> key_str;    // file name 
    string value;           // file value_name 泛型
    string spli_flag;       // 分隔符
    int is_repeat;          // vaue 时候重复
    int kv_id;              // 每个kv的id
};

//区分文件key 和索引key
const std::string flag_hash = "file_hash@";
class CFirstLevelDb
{
public:
    CFirstLevelDb();
    ~CFirstLevelDb();
    int init_db(const char* name_db, const char* config_name);
    int close_db();
private:

    int load_config(const char* file_name);

    tools::ESaveErrorCode put_new_kvs(const tools::SFileData& file_data);

	void add_flag_hash(std::string& hash_str);
	void del_flag_hash(std::string& hash_str);
	bool is_flag_key(const std::string& key_str);
    
public: 
    int delete_file(const std::string& key_string);

    tools::ESaveErrorCode put_new_file(tools::SFileData& file_data);

    tools::ESaveErrorCode update_file(const tools::SFileData* file_data);

    tools::ESaveErrorCode update_file_block_data(tools::SFileData& file_data, Json::Value& block_value);

    int get_message(const std::string& key_string, std::string& str_date);

	void get_all(std::map<string, string>& value_map);

private:
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::Status status;
    boost::filesystem::fstream file_stream;
    Json::FastWriter fwrite;
    Json::Reader reader;
    vector<SLevelSaveKV> v_save_kv;
};

