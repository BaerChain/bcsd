#include <tools.hpp>
#include <first_level_db.hpp>

class plugin_init : public appbase::plugin<plugin_init>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();
    
    int make_directories();
  private:
    bfs::path root_path;
};