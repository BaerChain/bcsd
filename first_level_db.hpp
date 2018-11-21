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
private:
    CFirstLevelDb();

	CFirstLevelDb(const CFirstLevelDb&);
	CFirstLevelDb& operator =(const CFirstLevelDb&);
    int close_db();
public:
	~CFirstLevelDb();
	//得到之前连接的实例 也有可能是没有连接 接下来初始化
	static CFirstLevelDb* get_single_level_db();
	//重新初始化 L0 的数据库参数 以前的连接将会close
	int init_db(const char* name_db, const char* config_name);
	bool is_open() { return nullptr != db;  }
public:
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

	tools::ESaveErrorCode insert_key_value(std::string key_string, const std::string value_string);

private:
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::Status status;
    boost::filesystem::fstream file_stream;
    Json::FastWriter fwrite;
    Json::Reader reader;
    vector<SLevelSaveKV> v_save_kv;
	
	//考虑线程安全
	//static pthread_mutex_t mutex;
	//static CFirstLevelDb p_level_db; //单利模式 
};
