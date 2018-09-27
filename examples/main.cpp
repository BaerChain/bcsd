#include <appbase/application.hpp>
#include <iostream>
#include <boost/exception/diagnostic_information.hpp>
#include <plugin_add.hpp>
#include <plugin_get.hpp>


int main( int argc, char** argv ) {
  try {
    appbase::app().register_plugin<plugin_add>();
    appbase::app().register_plugin<plugin_get>();
    if( !appbase::app().initialize( argc, argv ) )
       return -1;
    appbase::app().startup();
    appbase::app().exec();
  } catch ( const boost::exception& e ) {
    std::cerr << boost::diagnostic_information(e) << "\n";
  } catch ( const std::exception& e ) {
    std::cerr << e.what() << "\n";
  } catch ( ... ) {
    std::cerr << "unknown exception\n";
 }
    std::cout << "exited cleanly\n";
    return 0;
}
