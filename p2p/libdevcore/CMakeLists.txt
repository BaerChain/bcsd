file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_library(devcore ${sources} ${headers})

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils ../deps/include ../deps/lib)

target_link_libraries(devcore PUBLIC Boost::filesystem Boost::system Boost::log Boost::thread Threads::Threads PRIVATE ethash leveldb)

#find_package(LevelDB)
target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
target_link_libraries(devcore PRIVATE ${LEVELDB_LIBRARIES})
