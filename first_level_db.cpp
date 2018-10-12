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
    //verson 
    if (root.isMember("verson"))
    {
        sdata.verson = root["verson"].asString();
        cout << "get name verson::" <<sdata.verson<< endl;

    }
    //size
    if (root.isMember("size"))
    {
        sdata.size = root["size"].asInt();
        cout << "get name size::" <<sdata.size<< endl;

    }
    sdata.id_name = sdata.name + "_" + sdata.verson;

    cout << "get name idname::" << sdata.id_name << endl;

}

//加入新的文件信息
//测试版本 暂时使用json 所以参数都以string 为主
int CFirstLevelDb::put_new_file(std::string & message)
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
        return 0;
    }
    
    cout << "start check old file" << endl;
    //读取旧的kv
    std::string file_name_value;
    status = db->Get(leveldb::ReadOptions(), "name", &file_name_value);
    cout << "file_name_value :" << file_name_value << endl;
    //assert(status.ok());
    std::string file_verson_value;
    status = db->Get(leveldb::ReadOptions(), "verson", &file_verson_value);
   //assert(status.ok());

    //批量操作 原子
     // Batch atomic write.
    leveldb::WriteBatch batch;
    if (!file_verson_value.empty())
    {
        batch.Put(file_date.verson, file_name_value + "," + file_date.verson);
        cout << "up name index" << file_name_value + "," + file_date.verson<< endl;
    }
    if (!file_verson_value.empty())
    {
        batch.Put(file_date.name, file_verson_value + "," + file_date.verson);
        cout << "up verson index" << file_verson_value + "," + file_date.verson<< endl;

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
