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

class plugin_get : public appbase::plugin<plugin_get>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();
    
  private:
    string file_hash;
    string json_content;
    bfs::path store_path;
    bfs::path r_path;
    bfs::fstream json_file;
    Json::Reader root_reader;
    Json::Value node;
};