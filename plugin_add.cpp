#include <plugin_add.hpp>

void plugin_add::set_program_options( options_description& cli, options_description& cfg)
{
    // 这里的两个options_description是框架创建好了传过来的引用，可以直接使用
    // 这里是需要添加参数的位置
    cli.add_options()
           ("add_file", bpo::value<string>(), "想要加入的文件的文件路径" )
           ("add_help", "add file into local node")
           ;
}

void plugin_add::plugin_initialize( const variables_map& options )
{
    // 这里的variables_map也是框架已经给我们存好了的，里面存放了参数的信息，直接用就行
    // 获取需要的文件路径并保存路径
    this->file_path = options["add_file"].as<std::string>();
    
    // 获取文件的大小
    this->file_size = bfs::file_size(this->file_path);
    std::cout << this->file_size << std::endl;
    // 打开文件，为后续操作准备
    this->file_stream.open(this->file_path, std::ios::in | std::ios::binary);
}

void plugin_add::plugin_startup()
{
    std::cout << "starting chain plugin \n";
    // 计算整个文件的sha256的值并返回到re里
    char re[65] = "";
    sha_file(this->file_stream, re);
    re[64] = '\0';
    bfs::path copy_file_path = "./L1";
    copy_file_path /= re;   // 拼接整个文件改名的路径
    //复制文件到copy_file_path路径里，调用的是boost::filesystem提供的方法
    bfs::copy_file(file_path, copy_file_path);

    // 确定json文件的位置和名字，并打开json文件
    bfs::path json_path = "./L0";
    json_path /= re;
    json_path.replace_extension("json");
    json_file.open(json_path, std::ios::out);
    assert(json_file.is_open());

    // 写入原文件的绝对路径，文件名，大小
    node["path"] = bfs::system_complete(file_path).string();
    node["name"] = file_path.filename().string();
    node["size"] = file_size;
    node["hash"] = re;

    // 文件分块存入本地，文件名为sha256的值，并存入拿sha256作为分段路径的路径下
    cut_block();
}

void plugin_add::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
    string res = write_to_file.write(node); // 把json对象写入到一个string里
    std::cout << "json res is:" << res << std::endl;
    json_file << res;   //写入到文件里
    json_file.close();
    file_stream.close();
}

/***
 * 确定我们程序存储的根目录
 * @return < 0 说明执行失败
 */
int plugin_add::root_dir()
{
    
    return 0;
}

/***
 * 把文件拆分成1M大小的分块，并把分块写入对应规则的路径下
 * @return < 0 说明执行失败，成功时返回值是分块的数量
 */
int plugin_add::cut_block()
{
    file_stream.clear();
    file_stream.seekp(std::ios::beg);
    bfs::path block_name;
    bfs::fstream block_f;
    int i = 0;
    int left_file_size = this->file_size;
    char buf[BLOCK_SIZE] = "";
    while(1){
        memset(buf, 0, BLOCK_SIZE * sizeof(char));
        // 拼接出本次文件的名字
        string _block_name("block_");
        string num_s = boost::lexical_cast<string>(i);
        _block_name.append(num_s);
        std::cout << "block_name is " << _block_name << std::endl;
        
        // 判断当前还能读取的分块大小，当为最后一次时，剩余的数量是不大于我们指定的分块大小的
        int read_buf_size = (left_file_size >= BLOCK_SIZE ? BLOCK_SIZE : left_file_size);
        assert(file_stream.read(buf, read_buf_size));

        char res_hash[65] = "";
        sha_file_block(buf, res_hash, read_buf_size);
        res_hash[64] = '\0';
        std::cout << _block_name << " hash is :" << res_hash << std::endl;
        char cur_path[80] = "";
        sha_to_path(res_hash, cur_path);
        // 把文件块按序号写入json
        Json::Value item;
        item["num"] = i;
        item["value"] = res_hash;
        block.append(item);
        //block[num_s] = res_hash;

        // 把名字赋值给boost库的path类来管理
        block_name = r_path;
        block_name /= cur_path;
        block_name /= res_hash;
        block_f.open(block_name, std::ios::out | std::ios::binary);

        block_f.write(buf, read_buf_size);
        block_f.close(); //记得要关闭文件
        left_file_size = left_file_size - BLOCK_SIZE; //减去已经读了的分块大小就是剩余总的大小
        if(left_file_size <= 0) break;
        i++;
    }
    node["block"] = block;
    return i + 1;
}

/***
 * 把传入的文件做hash，然后传出结果
 * @参数fstream 需要打开这个文件，然后传入
 * @参数res 结果通过这个数组返回，需要65个字节
 * @return < 0 表示计算失败
 */
int plugin_add::sha_file(bfs::fstream& fp, char res[])
{
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char file_buf[BLOCK_SIZE] = "";

    // 循环读取传入的文件，目前每次读取量和分块是一样的大小，
    // 暂时还不知道这样对性能是否有影响
    while(!fp.eof()){
        fp.read(file_buf, BLOCK_SIZE);
        int read_size = fp.gcount();
        SHA256_Update(&sha256, file_buf, read_size);
    }
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(; i < 32; i++){
        sprintf(&res[i*2], "%0x",  hash[i]);
    }
    return 0;
}

/***
 * 把传入的buf做hash，然后传出结果
 * @参数buf 需要计算的块
 * @参数res 结果通过这个数组返回，需要65个字节
 * @参数buf_size 传入块的大小
 * @return < 0 表示计算失败
 */
int plugin_add::sha_file_block(char buf[], char res[], int buf_size)
{
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    // 我这里的块目前都是小于等于我们规定的块大小，所以不用循环，一次就行
    SHA256_Update(&sha256, buf, buf_size);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(; i < 32; i++){
        sprintf(&res[i*2], "%02x",  hash[i]);
    }
    return 0;
}

/***
 * 把hash值拆成4个一组的包含目录，并创建这些目录结构，如果存在就跳过进入下一层
 * 例子：传入0ecf942dc294fa9d2d3b7ac649d6e730ba3e0edb9b5eeec47c43c453c67b2ae9 这样的hash值
 *       返回0ecf/942d/c294/fa9d/2d3b/7ac6/49d6/e730/ba3e/0edb/9b5e/eec4/7c43/c453/c67b/2ae9 这样的路径，并建立每一级的文件夹
 * 注意：返回的路劲前后都没有'/'这个符号，所以返回的空间至少需要80个字节
 * @参数sha_val 需要用来建立目录的hash值，目前我们是64个字节，256位的
 * @参数res 创建好以后的目录最后的路径
 * @return < 0 表示计算失败
 */
int plugin_add::sha_to_path(char sha_val[], char res[])
{
    int i = 0;
    int cut = 4;
    r_path = "./L2";
    for(; i < 16; i++){
        bfs::path tmp = r_path;
        // 把hash值四个四个一组拿出来
        strncpy(&res[i * (cut + 1)], &sha_val[i * cut], cut);
        if(i == 15){
            res[(i + 1) * (cut + 1) - 1] = '\0';
        }else{
            res[(i + 1) * (cut + 1) - 1] = '/';
            res[(i + 1) * (cut + 1)] = '\0';
        }

        tmp /= res; // 加上存储的根目录组成完整的路径
        // std::cout << "path is :" << tmp << std::endl;
        if(!bfs::exists(tmp)){  // 判断目录是否存在
            // std::cout << "not exsits " << tmp << std::endl;
            bfs::create_directory(tmp);
        }
    }
    return 0;
}