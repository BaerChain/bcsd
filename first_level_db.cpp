#include "first_level_db.hpp"

CFirstLevelDb::CFirstLevelDb()
{
    db = nullptr;
}

CFirstLevelDb::~CFirstLevelDb()
{
    if (db)
    {
        delete db;
        db = nullptr;
    }
}


int CFirstLevelDb::init_db(const char* name_db, const char* config_name)
{
    options.create_if_missing = true;
    status = leveldb::DB::Open(options, name_db, &db);
    assert(status.ok());
    assert(load_config(config_name));
    return 0;
}

int CFirstLevelDb::close_db()
{
    if (db)
    {
        delete db;
        db = nullptr;
    }
    return 0;
}

int CFirstLevelDb::load_config(const char * file_name)
{
    Json::Reader reader;// 解析json用Json::Reader
    Json::Value root;

    std::ifstream fs;
    fs.open(file_name, std::ios::binary);
    if (reader.parse(fs, root, false))
    {
        std::string value_str;
        if (!root.isMember("key_value"))
        {
            std::cout<< "load config: "<<file_name <<" faild"<<std::endl;
            return 0;
        }
        int kv_size = root["key_value"].size();

        //读取每个kv设置
        std::cout << "load three kv item" << std::endl;
        for (int i = 0; i < kv_size; i++)
        {
            Json::Value kv_value = root["key_value"][i];
            if (!kv_value.isMember("key") 
                || !kv_value.isMember("value") 
                || !kv_value.isMember("name_slip_flag") 
                || !kv_value.isMember("is_repeat"))
            {
                std::cout << "the config's key is error please check!" << std::endl;
                return 0;
            }
            SLevelSaveKV s_kv;
            s_kv.key_str.clear();
            if (kv_value["key"].isArray())
            {
                int value_size = kv_value["key"].size();
                for (int j = 0; j < value_size; ++j)
                {
                    s_kv.key_str.push_back(kv_value["key"][j].asString());
                }
            }
            else
            {
                s_kv.key_str.push_back(kv_value["key"].asString());
            }
            s_kv.value = kv_value["value"].asString();
            s_kv.is_repeat = kv_value["is_repeat"].asInt();
            s_kv.spli_flag = kv_value["name_slip_flag"].asString();
            v_save_kv.push_back(s_kv);
        }
        cout << "load kv is  ok size:" << v_save_kv.size()<< endl;
    }
    return 1;
}

// 加入新的kvs add new file and add other correlation kv
Tools::ESaveErrorCode CFirstLevelDb::put_new_kvs(const Tools::SFileData& file_data)
{
    cout << "start put new kvs" << endl;
    int kv_size = v_save_kv.size();
    leveldb::WriteBatch batch;
    //Json::Reader reader;
    cout << "strat get keys..." << endl;
    for (int i = 0; i< kv_size; ++i)
    {
        string key_str = "";
        bool is_get_ok = true; // 标记 key 时候正确得到
        for (auto var : v_save_kv[i].key_str)
        {
            string temp_key = file_data.get_value(var);
            if (temp_key.empty())
            {
                cout <<"key:" << var <<" cant't find"<< endl;
                is_get_ok = false;
                break;
            }
            if (key_str.empty())
                key_str = temp_key;
            else
                key_str = key_str + v_save_kv[i].spli_flag + temp_key;

        }
        if (!is_get_ok || key_str.empty())
        {
            cout << "get the number:" << i << " key-value_item's key faild " << endl;
            continue;
        }
        //获取value 失败者 全部过程失败
        string temp_value = file_data.get_value(v_save_kv[i].value);
        if (temp_value.empty())
        {
            cout << "Error get value error" << endl;
            return Tools::ESaveErrorCode::e_value_error;
        }

        //已经获取到了 key - value
        //接下来必须全部成功 否则全部失败
        if (v_save_kv[i].is_repeat)
        {
            //存储多相同数据类型
            //查询时候存在
            Json::Value root;
            root.clear();
            string value_str = "";
            status = db->Get(leveldb::ReadOptions(), key_str, &value_str);
            is_get_ok = true;
            if (status.ok() && reader.parse(value_str, root))
            {
                //判断是否重复
                if (root.isMember(temp_value))
                {
                    is_get_ok = false;
                    cout << " the value have repeat" << endl;
                }
            }
            else
                root.clear();
            
            if (is_get_ok)
            {
                //添加数据
                root.append(temp_value);
                batch.Put(key_str, fwrite.write(root));
            }
        }
        else
        {
            batch.Put(key_str, temp_value);
        }
        cout << "kv store is ok and the kv's number:" << i << " key:" << key_str << " value:" << temp_value << endl;
    }
    status = db->Write(leveldb::WriteOptions(), &batch);
    // 失败可抛出异常
    //assert(status.ok());
    cout << "put file ok" << endl;
    //成功
    return Tools::ESaveErrorCode::e_no_error;
}

//对外接口 加入新的file
//计算file基本信息
Tools::ESaveErrorCode CFirstLevelDb::put_new_file(Tools::SFileData& file_data)
{
    cout << "start put new file" << endl;
    // 检查文件时候存在 hash 检验
    file_stream.open(file_data.file_name, std::ios::in | std::ios::binary);
    char hash_ret[65] = {0};
    Tools::sha_file(file_stream, hash_ret);
    file_data.file_hash = string(hash_ret);
    string temp_value;
    status = db->Get(leveldb::ReadOptions(), hash_ret, &temp_value);
    if (status.ok())
    {
        std::cout << "the file" << file_data.file_name << " is already exist!" << std::endl;
        return Tools::ESaveErrorCode::e_file_exit;
    }
    file_stream.close();
    //文件可以存储
    // 以json 格式存取 k-value
    // 写入原文件的绝对路径，文件名，大小，所属游戏名，所属游戏版本
    boost::filesystem::path file_path(file_data.file_name);
    Json::Value node;
    node["path"] = bfs::system_complete(file_path).string();
    node["name"] = file_path.filename().string();
    node["size"] =int( boost::filesystem::file_size(file_path));
    node["hash"] = file_data.file_hash;
    node["version"] = file_data.file_version;
    node["game_name"] = file_data.file_name;
    if (!file_data.base_file_name.empty() && !file_data.base_file_version.empty())
    {
        node["base_file_name"] = file_data.base_file_name;
        node["base_file_version"] = file_data.base_file_version;
    }

    file_data.file_value = fwrite.write(node);
    return put_new_kvs(file_data);
}

Tools::ESaveErrorCode CFirstLevelDb::update_file(const Tools::SFileData* file_data)
{
    //更新文件，只需要更新文件存储信息，不需要更新索引
    //检查文件hash 存在更新，不存在失败
    string temp_value;
    status = db->Get(leveldb::ReadOptions(),file_data->file_hash, &temp_value);
    if (status.ok())
    {
        std::cout << "the file" << file_data->file_name << " is already exist!" << std::endl;
        return Tools::ESaveErrorCode::e_file_exit;
    }
    status = db->Put(leveldb::WriteOptions(), file_data->file_hash,file_data->file_value);
    if (status.ok())
        return Tools::ESaveErrorCode::e_no_error;
    return Tools::ESaveErrorCode::e_leveldb_save_error;

}


Tools::ESaveErrorCode CFirstLevelDb::update_file_block_data(Tools::SFileData& file_data, Json::Value& block_value)
{
    Json::Value root;
    if (!reader.parse(file_data.file_value, root) 
    {
        cout<< "Error parse json value for file_block_date is error!" <<endl;
            return Tools::ESaveErrorCode::e_json_value_error;
    }
    if(root.isMember("block"))
        root.removeMember("block");
    root["block"] = fwrite.write(block_value); 
    file_data.file_value = fwrite.write(root);
    return update_file(file_data);
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
