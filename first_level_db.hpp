#pragma once
#include<iostream>
#include <jsoncpp/json/json.h>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

using namespace std;

/*
    提供leveldb存储查询服务
	存储传入文件 
    建立需要的索引
    提供对外的增删改接口
*/

struct SFirstLevelDb 
{
    std::string id_name;
    std::string name;
    std::string value;
    std::string verson;
    int size;

    SFirstLevelDb()
    {
        id_name = "";
        name = "";
        value = "";
        verson = "";
        size = 0;
    }
};

class CFirstLevelDb
{
public:
    CFirstLevelDb();
    ~CFirstLevelDb();
    int init_db(const std::string name_db);
    int close_db();
private:
    int put_new_kv(std::string& message);
    int delete_kv(const std::string& key_string);
    int update_kv(std::string& message);
    int update_file(SFirstLevelDb& file_data);

    void  file_str_sdate(std::string& file_str, SFirstLevelDb& sdata);

public:
    int put_new_file( std::string& message);
    int update_file( std::string& up_message);
    int delete_file(const std::string& key_string);
    int get_message(const std::string& key_string, std::string& str_date);

private:
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::Status status;
};