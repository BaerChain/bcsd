#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <set>
#include <typeinfo>

namespace bfs = boost::filesystem;
namespace ba = boost::asio;

enum enum_message_type {type_of_file, type_of_message};

// 现在只处理了包块顺序的问题，丢包问题还没处理
struct message_block
{
	int index;                          // 块序号
    int size;                           // value的大小，不包括\0
    bool is_end;                        // 传文件时是否为最后一块 true:是最后一块 false:不是
    char file_name[65];                 // 被传文件的文件名
    enum_message_type message_type;     // 消息类型 type_of_file:传文件 type_of_message:传字符串
	char value[1024];                   // 消息内容(可能是二进制数据，可能是字符串)

    //放set时按index从小到大排序
	bool operator < (const message_block & mb) const
	{
		return mb.index > this->index;
	}
};

struct node_info
{
    char node_id[65];
    ba::ip::udp::endpoint node_endpoint;
};



class peer
{
  public:
    // 连接到其它节点
    void connect_peer(std::string other_peer_ip, unsigned short other_peer_port);
    void connect_peer(ba::ip::udp::endpoint other_endpoint);
    peer(ba::io_service &ios, unsigned short peer_port);

    // 传输数据
    int transfer_data(void * buf, int len);
    int transfer_data(void * buf, int len, ba::ip::udp::endpoint this_ask_endpoint);
    int transfer_file(bfs::path transfer_file_path);
    int transfer_file(bfs::path transfer_file_path, ba::ip::udp::endpoint othre_node_endpoint);

    // 把接收的工作丢给系统
    void session_receive();
    // 接收到数据后系统回调的函数，用来处理接收到的数据
    void process_receive(const boost::system::error_code &ec);

    // 发送字符串消息
    int send_string_message(const char * string_message, ba::ip::udp::endpoint other_node_endpoint);

    // 把node：命令传输过来的节点id和对应的endpoint存到当前程序
    int insert_node(std::string &node_info);
    bfs::path store_path;
    ba::io_service &io_service_con;
    std::string node_id;
  private:
    ba::ip::udp::endpoint ourself_endpoint;
    ba::ip::udp::endpoint other_peer_send_endpoint;
    ba::ip::udp::endpoint other_peer_receive_endpoint;
    ba::ip::udp::socket _receive_socket;
    //ba::ip::udp::socket _send_socket;
    std::set<message_block> file_set;
    char receive_buf[1500];
    std::map<std::string, ba::ip::udp::endpoint> list_node_endpoint;
};

void get_input(peer * pr);