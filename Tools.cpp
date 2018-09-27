#include <Tools.hpp>

/***
 * 把传入的文件做hash，然后传出结果
 * @参数fstream 需要打开这个文件，然后传入
 * @参数res 结果通过这个数组返回，需要65个字节
 * @return < 0 表示计算失败
 */
int Tools::sha_file(bfs::fstream& fp, char res[])
{
    fp.clear();
    fp.seekp(std::ios::beg);
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char file_buf[BLOCK_SIZE] = "";

    // 循环读取传入的文件，目前每次读取量和分块是一样的大小，
    // 暂时还不知道这样对性能是否有影响
    while(!fp.eof()) {
        fp.read(file_buf, BLOCK_SIZE);
        int read_size = fp.gcount();
        SHA256_Update(&sha256, file_buf, read_size);
    }
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(; i < 32; i++) {
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
int Tools::sha_file_block(char buf[], char res[], int buf_size)
{
    unsigned char hash[SHA256_DIGEST_LENGTH] = "";
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    // 我这里的块目前都是小于等于我们规定的块大小，所以不用循环，一次就行
    SHA256_Update(&sha256, buf, buf_size);
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(; i < 32; i++) {
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
int Tools::sha_to_path(char sha_val[], char res[])
{
    int i = 0;
    int cut = 4;
    bfs::path r_path = "./L2";
    for(; i < 16; i++) {
        bfs::path tmp = r_path;
        // 把hash值四个四个一组拿出来
        strncpy(&res[i * (cut + 1)], &sha_val[i * cut], cut);
        if(i == 15) {
            res[(i + 1) * (cut + 1) - 1] = '\0';
        } else {
            res[(i + 1) * (cut + 1) - 1] = '/';
            res[(i + 1) * (cut + 1)] = '\0';
        }

        tmp /= res; // 加上存储的根目录组成完整的路径
        // std::cout << "path is :" << tmp << std::endl;
        if(!bfs::exists(tmp)) {  // 判断目录是否存在
            // std::cout << "not exsits " << tmp << std::endl;
            bfs::create_directory(tmp);
        }
    }
    return 0;
}
