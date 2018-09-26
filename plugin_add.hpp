#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <jsoncpp/json/json.h>
#include <openssl/sha.h>

#define BLOCK_SIZE (1024 * 1024)

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

using bpo::options_description;
using bpo::variables_map;
using std::string;
using std::vector;

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
    int sha_file(bfs::fstream& fp, char res[]);
    int sha_file_block(char buf[], char res[], int buf_size);
    int sha_to_path(char sha_val[], char res[]);
    
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