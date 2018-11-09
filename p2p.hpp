#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <set>
#include <typeinfo>

enum enum_message_type {type_of_file, type_of_message};

// 现在只处理了包块顺序的问题，丢包问题还没处理
struct message_block
{
	int index;                          // 块序号
    int size;                           // value的大小，不包括\0
    bool is_end;                        // 传文件时是否为最后一块 true:是最后一块 false:不是
    char file_name[33];                 // 被传文件的文件名
    enum_message_type message_type;     // 消息类型 type_of_file:传文件 type_of_message:传字符串
	char value[1024];                   // 消息内容(可能是二进制数据，可能是字符串)

    //放set时按index从小到大排序
	bool operator < (const message_block & mb) const
	{
		return mb.index > this->index;
	}
};

namespace bfs = boost::filesystem;
namespace ba = boost::asio;

class peer
{
  public:
    void connect_peer(std::string other_peer_ip, unsigned short other_peer_port);
    void connect_peer(ba::ip::udp::endpoint other_endpoint);
    peer(ba::io_service &ios, unsigned short peer_port);
    int transfer_data(void * buf, int len);
    int transfer_file(bfs::path transfer_file_path);
    void session_receive();
    void process_receive(const boost::system::error_code &ec);

    bfs::path store_path;
    ba::io_service &io_service_con;
  private:
    ba::ip::udp::endpoint ourself_endpoint;
    ba::ip::udp::endpoint other_peer_send_endpoint;
    ba::ip::udp::endpoint other_peer_receive_endpoint;
    ba::ip::udp::socket _receive_socket;
    ba::ip::udp::socket _send_socket;
    std::set<message_block> file_set;
    char receive_buf[1500];
};

void get_input(peer * pr);