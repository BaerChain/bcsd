# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

cmake_minimum_required(VERSION 3.5.1)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

project(ap2p)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
#set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY On)


#set(cryptopp_DIR "./include/cryptopp")
set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED ON)
#hunter_add_package(Boost COMPONENTS program_options filesystem system thread context fiber log)
#SET(BOOST_ROOT "/usr/local/")
find_package(Boost REQUIRED program_options filesystem system thread context fiber log_setup log)


#如果找到这个包，则可以通过在工程的顶层目录中的CMakeLists.txt 文件添加 include_directories(<NAME>_INCLUDE_DIRS) 
#来包含库的头文件，添加target_link_libraries(源文件 <NAME>_LIBRARIES)命令将源文件与库文件链接起来。
#find_package(cryptopp CONFIG REQUIRED)
#find_package(libscrypt CONFIG REQUIRED)
#find_package(ethash CONFIG REQUIRED)

include(ProjectSecp256k1)
include(ProjectLibFF)

find_package(Threads)

#Global include path for all libs.
include_directories("${CMAKE_SOURCE_DIR}")

add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)

ADD_EXECUTABLE(p2pTest ${DIR_SRCS})
target_include_directories(p2pTest SYSTEM PUBLIC ${Boost_INCLUDE_DIR} ./deps/include ./deps/lib )
target_link_libraries( p2pTest ${Boost_LIBRARIES} devcore devcrypto p2p libethash.a libcryptopp.a libscrypt.a)
