#pragma once

#include <tools.hpp>
#include <first_level_db.hpp>

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
    int set_initialize(bfs::path add_file_path, std::string add_file_belong_game, std::string add_file_version);
    std::string get_file_hash();

  private:
    CFirstLevelDb* leveldb_control;
    tools::SFileData _file_data;
    bfs::path leveldb_path;
    bfs::path config_path;
    int file_size;
    bfs::path file_path;
    string game_name_string;
    string game_version_string;
    bfs::fstream file_stream;
    bfs::path root_path;
    bfs::fstream json_file;
    Json::Value node;
    Json::FastWriter write_to_file;
    Json::Value block;
};