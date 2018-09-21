#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

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
    int cut_block();
    
  private:
    int file_size;
    bfs::path file_path;
    bfs::fstream file_stream;
};