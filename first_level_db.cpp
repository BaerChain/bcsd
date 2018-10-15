#include "first_level_db.hpp"

CFirstLevelDb::CFirstLevelDb()
{
    db = NULL;
}

CFirstLevelDb::~CFirstLevelDb()
{
    if (db)
    {
        delete db;
        db = NULL;
    }
}


int CFirstLevelDb::init_db(const std::string name_db)
{
    options.create_if_missing = true;
    status = leveldb::DB::Open(options, name_db, &db);
    assert(status.ok());
    return 0;
}

int CFirstLevelDb::close_db()
{
    if (db)
    {
        delete db;
        db = NULL;
    }
    return 0;
}

//加入新的kv
//目前测试版本暂时不使用此接口
int CFirstLevelDb::put_new_kv(std::string& message)
{
    return 0;
}

//删除kv
//目前测试版本暂时不使用此接口
int CFirstLevelDb::delete_kv(const std::string& key_string)
{
    return 0;
}

//调用upde 覆盖更新kv
//目前测试版本暂时不使用此接口
int CFirstLevelDb::update_kv(std::string& message)
{
    return 0;
}


void CFirstLevelDb::file_str_sdate(std::string& file_str, SFirstLevelDb& sdata)
{
    // 将message 分析为kv
    Json::Reader reader;
    Json::Value root;
    //从message中读取data
    if (!reader.parse(file_str, root))
    {
        std::cout << "parse jsonstr error \n" << file_str << std::endl;
        assert(NULL);
    }

    //获取相关属性 
    /*前期直接写属性，
        后期可考虑配置文档
    */
    //value
    sdata.value = file_str;
    // name 
    if (root.isMember("name"))
    {
        sdata.name = root["name"].asString();
        cout << "get name ok::" <<sdata.name << endl;
    }
    //version 
    if (root.isMember("version"))
    {
        sdata.version = root["version"].asString();
        cout << "get name version::" <<sdata.version<< endl;

    }
    //size
    if (root.isMember("size"))
    {
        sdata.size = root["size"].asInt();
        cout << "get name size::" <<sdata.size<< endl;

    }
    sdata.id_name = sdata.name + "_" + sdata.version;

    cout << "get name idname::" << sdata.id_name << endl;

}

//更新索引kv时，检查重复等等
//目前恒为真
bool CFirstLevelDb::get_index_value(const std::string & key_str, const std::string & value_str, std::string & new_value_str)
{
    new_value_str = "";
    status = db->Get(leveldb::ReadOptions(), key_str, &new_value_str);
    if (!new_value_str.empty())
    {
       // 判断是否 存在 value_str
       if (0)
       {
           return false;
       }
       new_value_str = new_value_str + "," + value_str;
    }
    new_value_str = value_str; 
    return true;
}

//加入新的文件信息
//测试版本 暂时使用json 所以参数都以string 为主
int CFirstLevelDb::put_new_file(std::string& message)
{
    //文件属性结构转换
    SFirstLevelDb file_date;
    file_str_sdate(message, file_date);
    //写入db
    /*
    属性kv注意 不能直接覆盖旧的kv   
    */
    std::string file_value;
    status = db->Get(leveldb::ReadOptions(), file_date.id_name, &file_value);
    //assert(status.ok());
    if (status.ok() && !file_value.empty())
    {
        //有旧文件 更新操作
        update_file(file_date);
        std::cout << "更新现有文件" << std::endl;
        return 0;
    }
    
    cout << "start check old file" << endl;
    //读取旧的kv
    //批量操作 原子
    // Batch atomic write.
    leveldb::WriteBatch batch;
    std::string new_value;
    if (get_index_value(file_date.name, file_date.version, new_value))
    {
        batch.Put(file_date.name, new_value );
        cout << "up name index:" << new_value << endl;
    }
    if (get_index_value(file_date.version, file_date.name, new_value))
    {
        batch.Put(file_date.version, new_value );
        cout << "up version index:" << new_value << endl;
    }

    cout << "id_name:" << file_date.id_name << endl;
    cout << "file_value:" << file_date.id_name << endl;

    batch.Put(file_date.id_name, file_date.value);

    status = db->Write(leveldb::WriteOptions(), &batch);
    // 失败可抛出异常
    //assert(status.ok());
    cout << "put file ok" << endl;
    return 0;
}
int CFirstLevelDb::update_file(std::string & up_message)
{
    SFirstLevelDb file_data;
    file_str_sdate(up_message, file_data);

    return update_file(file_data);
}

int CFirstLevelDb::update_file(SFirstLevelDb& file_data)
{
    status = db->Put(leveldb::WriteOptions(), file_data.id_name, file_data.value);
    //assert(status.ok());
    return 1;
}

// key_string 查询key值
// str_date 输出查询结果
int CFirstLevelDb::get_message(const std::string & key_string, std::string& str_date)
{
    status = db->Get(leveldb::ReadOptions(), key_string, &str_date);
    //assert(status.ok());
    return 0;
}
int CFirstLevelDb::delete_file(const std::string& key_string)
{
    status = db->Delete(leveldb::WriteOptions(), key_string);
    //assert(status.ok());
    return 0;
}
