#include <plugin_check.hpp>

void plugin_check::set_program_options( options_description& cli, options_description& cfg) override
{
    cli.add_options()
            ("offset", bpo::value<long long>(), "offset num of begin")
            ("length", bpo::value<long long>(), "length of calculate")
            ;
    }
}

void plugin_check::plugin_initialize( const variables_map& options )
{
    if(options.count("offset")) {
        offset_of_file = options["offset"].as<long long>();
        length_of_calculate = options["length"].as<long long>();
    }
}

void plugin_check::plugin_startup()
{
    get_offset_hash();
}

void plugin_check::plugin_shutdown()
{

}