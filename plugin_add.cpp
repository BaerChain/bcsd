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
    std::cout << this->file_path << std::endl;
    // 获取文件的大小
    this->file_size = bfs::file_size(this->file_path);
    std::cout << this->file_size << std::endl;
    // 打开文件，为后续操作准备
    file_stream.open(this->file_path, std::ios::in | std::ios::binary);
    char re[65] = "";
    sha_file(file_stream, re);
    re[64] = '\0';
    std::cout << std::endl << re << std::endl;
    cut_block();
}

void plugin_add::plugin_startup()
{
    std::cout << "starting chain plugin \n";
}

void plugin_add::plugin_shutdown()
{
    std::cout << "shutdown chain plugin \n";
}

/***
 * 把文件拆分成1M大小的分块
 * @return < 0 说明执行失败，成功时返回值是分块的数量
 */
int plugin_add::cut_block()
{
    bfs::path block_name;
    bfs::fstream block_f;
    int i = 1;
    int left_file_size = this->file_size;
    char buf[BLOCK_SIZE];
    while(1)
    {
        // 拼接出本次文件的名字
        string _block_name("block_");
        string num_s = boost::lexical_cast<string>(i);
        _block_name.append(num_s);
        std::cout << "block_name is " << _block_name << std::endl;
        
        // 把名字赋值给boost库的path类来管理
        block_name = _block_name;
        block_f.open(block_name, std::ios::out | std::ios::binary);

        // 判断当前还能读取的分块大小，当为最后一次时，剩余的数量是不大于我们指定的分块大小的
        int read_buf_size = (left_file_size >= BLOCK_SIZE ? BLOCK_SIZE : left_file_size);
        file_stream.read(buf, read_buf_size);
        block_f.write(buf, read_buf_size);
        block_f.close(); //记得要关闭文件
        left_file_size = left_file_size - BLOCK_SIZE; //减去已经读了的分块大小就是剩余总的大小
        if(left_file_size <= 0) break;
        i++;
    }
    return i;
}

int plugin_add::sha_file(bfs::fstream& fp, char res[])
{
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char file_buf[BLOCK_SIZE] = "";
    while(!fp.eof())
    {
        fp.read(file_buf, BLOCK_SIZE);
        int read_size = fp.gcount();
        SHA256_Update(&sha256, file_buf, read_size);
    }
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(; i < 32; i++)
    {
        sprintf(&res[i*2], "%0x",  hash[i]);
    }
    return 0;
}