#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <set>
#include <typeinfo>
#include <tools.hpp>
#include <first_level_db.hpp>
#include <plugin_add.hpp>

//namespace bfs = boost::filesystem;
namespace ba = boost::asio;

enum enum_message_type {type_of_file, type_of_message, kv_data, check,};

// 现在只处理了包块顺序的问题，丢包问题还没处理
struct message_block
{
	int index;                          // 块序号
    int max_index;                      // 最大序号：当块序号等于最大序号，表示文件传输完成，最大序号还可用于检测文件是否丢包
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

struct content_info
{
    int content_size = 0;
    enum_message_type message_type = type_of_message;     // 消息类型 type_of_file:传文件 type_of_message:传字符串 kv_data:传键值对
    char key[128] = "";
    char content[1024 * 1024] = "";
};

struct node_info
{
    char node_id[65];
    ba::ip::udp::endpoint node_endpoint;
};

struct file_info
{
    bool exist_flag = false;
    std::set<message_block> file_set;
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
    void transfer_tcp_file(ba::ip::tcp::socket & client_socket, bfs::path file_path);
    void transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message);
    void transfer_tcp_string(ba::ip::tcp::socket &client_socket, std::string message, ba::ip::tcp::endpoint &target_endpoint);
    void transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message, enum_message_type type);

    // 把接收的工作丢给系统
    void session_udp_receive();
    void session_tcp_receive();
    // 接收到数据后系统回调的函数，用来处理接收到的数据
    void process_receive(const boost::system::error_code &ec);
    void tcp_process_link(ba::ip::tcp::socket * new_socket, const boost::system::error_code &ec);
    void tcp_process_receive(ba::ip::tcp::socket * current_socket);
    // 获取文件hash请求对方，先获得对应的value，再依次获取文件块
    void tcp_get_value_and_block(std::string file_hash, ba::ip::tcp::endpoint other_peer_server_endpoint);

    // 发送字符串消息
    int send_string_message(const char * string_message, ba::ip::udp::endpoint other_node_endpoint);
    // 依次给当前节点列表中的节点发送消息
    int udp_send_in_order(std::string message);

    // 读取配置文件里的公共节点的数据
    int load_config(bfs::path config_path);

    // 同步L0
    int keep_same_leveldb();
    // 获取json中的块文件
    int get_file_in_key(std::string root_json, ba::ip::tcp::endpoint current_point);

	//check 
	std::string challenge(ba::ip::tcp::socket& _socket, ba::ip::tcp::endpoint &target_endpoint, const std::string& _order);
	std::string challenge(ba::ip::tcp::socket& _socket, const std::string _nodeid, const std::string _order);

    // udp::endpoint 转 tcp::endpoint
    int udp2tcp(ba::ip::udp::endpoint &src, ba::ip::tcp::endpoint &des);
    // tcp::endpoint 转 udp::endpoint
    int tcp2udp(ba::ip::tcp::endpoint &src, ba::ip::udp::endpoint &des);

    // 把node：命令传输过来的节点id和对应的endpoint存到当前程序
    int insert_node(std::string &node_info);
    bfs::path store_path;
    ba::io_service &io_service_con;
    std::string node_id;
    ba::ip::tcp::endpoint tcp_ourself_endpoint;

  private:
    ba::ip::udp::endpoint ourself_endpoint;
    
    ba::ip::udp::endpoint other_peer_send_endpoint;
    ba::ip::udp::endpoint other_peer_receive_endpoint;
    ba::ip::udp::socket _receive_socket;
    //ba::ip::tcp::socket server_socket;
    ba::ip::tcp::acceptor server_acceptor;
    //ba::ip::udp::socket _send_socket;
    std::set<message_block> file_set;
    char receive_buf[1500];
    std::map<std::string, ba::ip::udp::endpoint> list_node_endpoint;
    std::map<std::string, file_info> list_file;

    // leveldb需要的
    CFirstLevelDb* leveldb_control;
    bfs::path leveldb_path;
    bfs::path config_path;
};

void get_input(peer * pr);