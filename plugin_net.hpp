#include <tools.hpp>
#include <p2p.hpp>

class plugin_net : public appbase::plugin<plugin_net>
{
  public:
    APPBASE_PLUGIN_REQUIRES();
    
    virtual void set_program_options( options_description& cli, options_description& cfg) override;

    void plugin_initialize( const variables_map& options );
    void plugin_startup();
    void plugin_shutdown();

    
  private:
    string peer_or_server;
    string ip_address;
    unsigned short _port;
    
};