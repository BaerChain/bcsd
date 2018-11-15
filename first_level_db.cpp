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
    cout << " start load kv-config" << endl;
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
    if (v_save_kv.empty())
    {
        cout << "load kv-config faid and pease check!" << endl;
        return 0;
    }
    return 1;
}

// 加入新的kvs add new file and add other correlation kv
tools::ESaveErrorCode CFirstLevelDb::put_new_kvs(const tools::SFileData& file_data)
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
            cout << "get the number:" << i << " key-value_item's key faild  and continue next!"  << endl;
            continue;
        }
        //获取value 失败则跳过这个index ，继续下一个index存储
        string temp_value = file_data.get_value(v_save_kv[i].value);
        if (temp_value.empty())
        {
            cout << " get value:"<< v_save_kv[i].value << " is empty and continue next!" << endl;
            continue;
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
    return tools::ESaveErrorCode::e_no_error;
}

void CFirstLevelDb::add_flag_hash(std::string & hash_str)
{
	hash_str = flag_hash + hash_str;
}

void CFirstLevelDb::del_flag_hash(std::string & hash_str)
{
	if (hash_str.length() <= flag_hash.length())
		return;
	//*****从i开始长度为pos-i的子字符串
	int str_length = hash_str.length();
	string str = hash_str.substr(flag_hash.length()-1, str_length - flag_hash.length());
	hash_str = str;
}

bool CFirstLevelDb::is_flag_key(const std::string& key_str)
{
	if (key_str.length() <= flag_hash.length())
	{
		cout << "size error" << endl;
		return	false;
	}
	string str = key_str.substr(0, flag_hash.length() - 1);
	if (str == flag_hash)
	{
		return true;
	}
	cout << " not peer" << endl;
	return false;
}

//对外接口 加入新的file
//计算file基本信息
tools::ESaveErrorCode CFirstLevelDb::put_new_file(tools::SFileData& file_data)
{
    cout << "start put new file" << endl;
    // 检查文件时候存在 hash 检验
    file_stream.open(file_data.file_name, std::ios::in | std::ios::binary);
    char hash_ret[65] = {0};
    tools::sha_file(file_stream, hash_ret);
    file_data.file_hash = string(hash_ret);
    string temp_value;
    status = db->Get(leveldb::ReadOptions(), hash_ret, &temp_value);
    if (status.ok())
    {
        std::cout << "the file " << file_data.file_name << " is already exist!" << std::endl;
        return tools::ESaveErrorCode::e_file_exit;
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
    //std::cout << "file value in leveldb is: " << file_data.file_value << std::endl;

	//添加flag
	add_flag_hash(file_data.file_hash);
    return put_new_kvs(file_data);
}

tools::ESaveErrorCode CFirstLevelDb::update_file(const tools::SFileData* file_data)
{
    //更新文件，只需要更新文件存储信息，不需要更新索引
    //检查文件hash 存在更新，不存在失败
    string temp_value;
    status = db->Get(leveldb::ReadOptions(),file_data->file_hash, &temp_value);
    if (!status.ok())
    {
        std::cout << "the file" << file_data->file_name << " is not exist!" << std::endl;
        return tools::ESaveErrorCode::e_file_exit;
    }
    //std::cout << "file_data->file_value is: " << file_data->file_value << std::endl;
    status = db->Put(leveldb::WriteOptions(), file_data->file_hash,file_data->file_value);
    if (status.ok())
        return tools::ESaveErrorCode::e_no_error;
    return tools::ESaveErrorCode::e_leveldb_save_error;

}


tools::ESaveErrorCode CFirstLevelDb::update_file_block_data(tools::SFileData& file_data, Json::Value& block_value)
{
    Json::Value root;
    std::cout << "file data is: " << file_data.file_value << std::endl;
    if (!reader.parse(file_data.file_value, root))
    {
        cout<< "Error parse json value for file_block_date is error!" <<endl;
            return tools::ESaveErrorCode::e_json_value_error;
    }
    if(root.isMember("block"))
        root.removeMember("block");
    root["block"] = block_value;
    file_data.file_value = fwrite.write(root);

	//add flag
	add_flag_hash(file_data.file_hash);
    return update_file(&file_data);
}

// key_string 查询key值
// str_date 输出查询结果
int CFirstLevelDb::get_message(const std::string & key_string, std::string& str_date)
{
	// 判断key是否添加了 flag 

    status = db->Get(leveldb::ReadOptions(), key_string, &str_date);
	if (status.ok())
		return 0;
	//加 falg
	str_date.clear();
	std::string str_key = key_string;
	add_flag_hash(str_key);
	status = db->Get(leveldb::ReadOptions(), str_key, &str_date);

    return 0;
}
void CFirstLevelDb::get_all(std::map<string, string>& value_map)
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		//cout << it->key().ToString() << ": " << it->value().ToString() << endl;
		if (is_flag_key(it->key().ToString()))
		{
			string map_key = it->key().ToString();
			del_flag_hash(map_key);
			value_map[map_key] = it->value().ToString();

			cout << value_map[map_key] << endl;
		}
	}
	//assert(it->status().ok());  // Check for any errors found during the scan
	delete it;
}

tools::ESaveErrorCode CFirstLevelDb::insert_key_value(std::string key_string, const std::string value_string)
{
	add_flag_hash(key_string);
	status = db->Put(leveldb::WriteOptions(), key_string, value_string);
	if (status.ok())
		return tools::ESaveErrorCode::e_no_error;
	return tools::ESaveErrorCode::e_leveldb_save_error;
}
int CFirstLevelDb::delete_file(const std::string& key_string)
{
    status = db->Delete(leveldb::WriteOptions(), key_string);
    //assert(status.ok());
    return 0;
}
