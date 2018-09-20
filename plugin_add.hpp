#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>


namespace bpo = boost::program_options;

using bpo::options_description;
using bpo::variables_map;
using std::string;
using std::vector;

class plugin_add : public appbase::plugin<plugin_add>
{
  public:
    plugin_add() : file_size(0), file_name("tom"){};
    APPBASE_PLUGIN_REQUIRES();

    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();
  private:
    int file_size;
    string file_name;
    string file_path;

};