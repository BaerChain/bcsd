#pragma once

#include <Tools.hpp>

class plugin_add : public appbase::plugin<plugin_add>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();

    int root_dir();
    int cut_block();
    
  private:
    int file_size;
    bfs::path file_path;
    bfs::fstream file_stream;
    bfs::path r_path;
    bfs::fstream json_file;
    Json::Value node;
    Json::FastWriter write_to_file;
    Json::Value block;
};