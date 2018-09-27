#include <Tools.hpp>

class plugin_get : public appbase::plugin<plugin_get>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();

    int download_block();
    int store_file_into_path();
  private:
    bool is_local;
    string file_hash;
    string json_content;
    bfs::path store_path;
    bfs::path store_file_path;
    bfs::path r_path;
    bfs::fstream json_file;
    Json::Reader root_reader;
    Json::Value node;
};