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

namespace Tools {
    int sha_file(bfs::fstream& fp, char res[]);
    int sha_file_block(char buf[], char res[], int buf_size);
    int sha_to_path(char sha_val[], char res[]);
}