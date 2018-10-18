#include <tools.hpp>
#include <first_level_db.hpp>

class plugin_check : public appbase::plugin<plugin_check>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();
    
    void get_offset_hash();
    void get_block_offset_hash();
  private:
    CFirstLevelDb leveldb_control;  // 数据库接口
    bfs::path check_file_hash;      // 需要校验的文件的hash
    long long offset_of_file;       // 从文件头开始的偏移量
    long long length_of_calculate;  // 需要计算的数据长度
    bfs::path root_path;            // 项目存储根目录
    string json_content;            // 存放json内容
    Json::Reader root_reader;       // json解析器
    Json::Value node;               // json
};