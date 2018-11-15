#include <p2p.hpp>

// 构造函数，生成接收端的socket，然后把接收的回调函数提给系统
peer::peer(ba::io_service &ios, unsigned short peer_port) 
    : io_service_con(ios)
    , _receive_socket(ios, ba::ip::udp::endpoint(ba::ip::udp::v4(), peer_port))
    //, _send_socket(ios)
{
    std::cout << peer_port << std::endl;
    leveldb_path = "./local";
    config_path = "../kv_config.json";
    leveldb_control.init_db(leveldb_path.string().c_str(), config_path.string().c_str());
    session_receive();
}

// 异步执行接收数据的任务
void peer::session_receive()
{
    memset(receive_buf, 0, 1500);
	_receive_socket.async_receive_from(
		boost::asio::buffer(receive_buf),
		other_peer_send_endpoint,
		boost::bind(&peer::process_receive,
		this,
		boost::asio::placeholders::error));
    //_send_socket.open(ba::ip::udp::v4());
}

// 处理接收到的数据
void peer::process_receive(const boost::system::error_code &ec)
{
    //std::cout << "peer address:" << other_peer_send_endpoint.address().to_string() << std::endl;
    //std::cout << "peer port:" << other_peer_send_endpoint.port() << std::endl;
    std::cout << "receive_data sock:" << _receive_socket.local_endpoint().address().to_string() << ":" << _receive_socket.local_endpoint().port() << std::endl;
    std::cout << "receive_data server:" << other_peer_send_endpoint.address().to_string() << ":" << other_peer_send_endpoint.port() << std::endl;
    
    message_block mb;
    memcpy(&mb, receive_buf, sizeof(mb));
    if(mb.message_type == type_of_message){ // 如果传的是字符串
        std::cout << "this message is char" << std::endl;
        std::string string_temp(mb.value, mb.size);
        std::cout << string_temp << std::endl;
        if(strncmp(string_temp.c_str(), "getfile:", 8) == 0){   // 请求文件，在本地查找文件是否存在，存在返回消息，不存在就跳过
            std::cout << "in getfile" << std::endl;
            char str_file_path[127];
    		strcpy(str_file_path, string_temp.c_str());
            char *cmd = strtok(str_file_path, ":");
    		char *file_path_char = strtok(NULL, ":");
            std::string file_hash = file_path_char;
            char buf[256] = "";
            sprintf(buf, "exist:%s", file_path_char);
            string json_content;
            leveldb_control.get_message(file_hash, json_content);
            std::cout << "json_content:" << json_content << std::endl;
            if(!json_content.empty()){
                send_string_message(buf, other_peer_send_endpoint);
            }
            /*if(bfs::exists(file_path_char)){ // 本地文件校验存在与否
                send_string_message(buf, other_peer_send_endpoint);
            }*/
            /*std::cout << "file_path: " << file_path_char << std::endl;
            bfs::path file_path = file_path_char;
            transfer_file(file_path);
            std::cout << "file " << file_path_char << "is transfer complete!" << std::endl;*/
        }else if(strncmp(string_temp.c_str(), "transfer:", 9) == 0){    // 收到这个消息头表示正式开始传文件
            char str_file_path[127];
    		strcpy(str_file_path, string_temp.c_str());
            char *cmd = strtok(str_file_path, ":");
    		char *file_path_char = strtok(NULL, ":");
            std::cout << "file_path: " << file_path_char << std::endl;
            bfs::path file_path = file_path_char;
            string json_content;
            std::string file_hash = file_path_char;

            leveldb_control.get_message(file_hash, json_content);
            send_string_message(json_content.c_str(), other_peer_send_endpoint);
            //transfer_file(file_path, other_peer_send_endpoint);
            std::cout << "file " << file_path_char << " is transfer complete!" << std::endl;
        }else if(strncmp(string_temp.c_str(), "getnode:", 8) == 0){ // 这里表示同步本地节点的节点数据到请求的节点
            char buf[1024] = "";
            sprintf(buf, "node:%s:%s:%u", node_id.c_str(), _receive_socket.local_endpoint().address().to_string().c_str(), _receive_socket.local_endpoint().port());
            std::cout << "buf is:" << buf << std::endl;
            
            send_string_message(buf, other_peer_send_endpoint);
        }else if(strncmp(string_temp.c_str(), "getallnode:", 11) == 0){ // 这里表示同步本地节点存储的所有节点数据到请求的节点
            std::map<std::string, ba::ip::udp::endpoint>::iterator it;
            for(it = list_node_endpoint.begin(); it != list_node_endpoint.end(); it++){
                char buf[1024] = "";
                sprintf(buf, "node:%s:%s:%u", it->first.c_str(), 
                    it->second.address().to_string().c_str(), 
                    it->second.port());
                send_string_message(buf, other_peer_send_endpoint);
            }
        }else if(strncmp(string_temp.c_str(), "node:", 5) == 0){
            /*char node_info_str[1024] = "";
            strcpy(node_info_str, string_temp.c_str() + 5);
            char *node = strtok(node_info_str, ":");
            char *ip = strtok(NULL, ":");
    		char *port = strtok(NULL, ":");
            std::cout << "receive node is: " << node << " " << ip << " " << port << std::endl;
            list_node_endpoint.insert(std::pair<std::string, ba::ip::udp::endpoint>(node, other_peer_send_endpoint));*/
            insert_node(string_temp);
        }else if(strncmp(string_temp.c_str(), "exist:", 6) == 0){   // 对方回应文件存在，就去请求文件
            // 这里还需要后续收到的存在信号不再生效
            char buf[256] = "";
            sprintf(buf, "transfer:%s", string_temp.c_str() + 6);
            send_string_message(buf, other_peer_send_endpoint);
        }
    }else{ //如果传的是文件
        std::cout << "this message is binary" << std::endl;
        file_set.insert(mb);
        if(mb.is_end){
            bfs::path file_path = store_path;
            file_path /= mb.file_name;
            bfs::fstream store_stream;
            store_stream.open(file_path, std::ios::binary | std::ios::out);
            std::set<message_block>::iterator it;
            for(it = file_set.begin(); it != file_set.end(); ++it){
                
                std::cout << "for index:" <<it->index << std::endl;
                store_stream.write(it->value, it->size);
            }
            store_stream.close();
        }
    }
    session_receive();
}

// 连接到对等点
void peer::connect_peer(std::string other_peer_ip, unsigned short other_peer_port)
{
    ba::ip::udp::resolver _resolver(io_service_con);
	ba::ip::udp::resolver::query _query(ba::ip::udp::v4(), other_peer_ip.c_str(), boost::lexical_cast<std::string>(other_peer_port));
	other_peer_send_endpoint = *_resolver.resolve(_query);
    //other_peer_endpoint = 
	//_send_socket.open(ba::ip::udp::v4());
}
void peer::connect_peer(ba::ip::udp::endpoint other_receive_endpoint)
{
    std::cout << "connect_peer address:" << other_receive_endpoint.address().to_string() << std::endl;
    std::cout << "connect_peer port:" << other_receive_endpoint.port() << std::endl;
    other_peer_receive_endpoint = other_receive_endpoint;
	//_send_socket.open(ba::ip::udp::v4());
    send_string_message("getnode:", other_receive_endpoint);
    std::string _write_message;
    while(std::getline(std::cin, _write_message)){  // 获取用户和对等点交流的命令
        if(strncmp(_write_message.c_str(), "file:", 5) == 0){   // file命令表示传输文件，后面跟文件的路径，例如file:./text.txt
            char str_file_path[127];
    		strcpy(str_file_path, _write_message.c_str());
            char *cmd = strtok(str_file_path, ":");
    		char *file_path_char = strtok(NULL, ":");
            std::cout << "file_path: " << file_path_char << std::endl;
            bfs::path file_path = file_path_char;
            transfer_file(file_path);
        }else if(strncmp(_write_message.c_str(), ":out", 5) == 0){  // 退出当前连接，本地仍然存储，只是退出会话
            break;
        }else if(strncmp(_write_message.c_str(), "getfile:", 8) == 0){ //给当前维护的所有endpoint发送获取文件的消息
            std::map<std::string, ba::ip::udp::endpoint>::iterator it;
            for(it = list_node_endpoint.begin(); it != list_node_endpoint.end(); it++){
                send_string_message(_write_message.c_str(), it->second);
            }
        }else{
            message_block tmp;
            tmp.message_type = type_of_message;
            strcpy(tmp.value, _write_message.c_str());
            tmp.size = _write_message.length();
            transfer_data((void *)&tmp, sizeof(tmp));
            std::cout << "your send message is:" << tmp.value << std::endl;
        }
    }
}

// 发送二进制数据
int peer::transfer_data(void * buf, int len)
{
    std::cout << "transfer_data sock:" << _receive_socket.local_endpoint().address().to_string() << ":" << _receive_socket.local_endpoint().port() << std::endl;
    std::cout << "transfer_data server:" << other_peer_receive_endpoint.address().to_string() << ":" << other_peer_receive_endpoint.port() << std::endl;
    _receive_socket.send_to(boost::asio::buffer(buf, len), other_peer_receive_endpoint);
    return 0;
}

// 指定节点发送二进制数据
int peer::transfer_data(void * buf, int len, ba::ip::udp::endpoint this_ask_endpoint)
{
    std::cout << "transfer_data sock:" << _receive_socket.local_endpoint().address().to_string() << ":" << _receive_socket.local_endpoint().port() << std::endl;
    std::cout << "transfer_data server:" << this_ask_endpoint.address().to_string() << ":" << this_ask_endpoint.port() << std::endl;
    _receive_socket.send_to(boost::asio::buffer(buf, len), this_ask_endpoint);
    return 0;
}

// 发送文件
int peer::transfer_file(bfs::path transfer_file_path)
{
    bfs::path file_path = transfer_file_path;
    int file_size_int = bfs::file_size(file_path);
    bfs::fstream file_stream;
    file_stream.open(file_path, std::ios::binary | std::ios::in);
    int i = 0;
    int file_read_size = 0;
    while(1){
        char file_block_buf[1024];
        file_stream.read(file_block_buf, 1024);
        message_block tmp;
        tmp.index = i++;
        tmp.size = file_stream.gcount();
        file_read_size += tmp.size;
        tmp.message_type = type_of_file;
        std::cout << "read_size:" << file_read_size << " " << "size total:" << file_size_int << std::endl;
        if(file_read_size == file_size_int){
            tmp.is_end = true;
        }
        strcpy(tmp.file_name, file_path.filename().string().c_str());
        memcpy(tmp.value, file_block_buf, tmp.size);
        
        transfer_data((void *)&tmp, sizeof(tmp));
        if(tmp.is_end){
            break;
        }
    }
    return 0;
}
// 给指定的节点发送文件
int peer::transfer_file(bfs::path transfer_file_path, ba::ip::udp::endpoint othre_node_endpoint)
{
    std::cout << "transfer_file sock:" << _receive_socket.local_endpoint().address().to_string() << ":" << _receive_socket.local_endpoint().port() << std::endl;
    std::cout << "transfer_file server:" << othre_node_endpoint.address().to_string() << ":" << othre_node_endpoint.port() << std::endl;
    bfs::path file_path = transfer_file_path;
    int file_size_int = bfs::file_size(file_path);
    bfs::fstream file_stream;
    file_stream.open(file_path, std::ios::binary | std::ios::in);
    int i = 0;
    int file_read_size = 0;
    while(1){
        char file_block_buf[1024];
        file_stream.read(file_block_buf, 1024);
        message_block tmp;
        tmp.index = i++;
        tmp.size = file_stream.gcount();
        file_read_size += tmp.size;
        tmp.message_type = type_of_file;
        tmp.is_end = false;
        std::cout << "read_size:" << file_read_size << " " << "size total:" << file_size_int << std::endl;
        if(file_read_size == file_size_int){
            tmp.is_end = true;
        }
        strcpy(tmp.file_name, file_path.filename().string().c_str());
        memcpy(tmp.value, file_block_buf, tmp.size);
        std::cout << "end flag is:" << tmp.is_end << std::endl;
        transfer_data((void *)&tmp, sizeof(tmp), othre_node_endpoint);
        if(tmp.is_end){
            break;
        }
    }
    std::cout << "transfer_file is end" << std::endl;
    return 0;
}

// 给指定节点发送字符串消息
int peer::send_string_message(const char * string_message, ba::ip::udp::endpoint other_node_endpoint)
{
    std::cout << "in send_string_message" << std::endl;
    message_block tmp;
    tmp.message_type = type_of_message;

    // 后续处理消息超出目前的1024字节的问题
    tmp.index = 0;
    tmp.max_index = 0;
    strcpy(tmp.value, string_message);
    tmp.size = strlen(string_message);
    transfer_data((void *)&tmp, sizeof(tmp), other_node_endpoint);
    return 0;
}

// 把返回的节点id对应的ip地址和端口存到map里，暂时消息必须是node:开头并中间用冒号隔开的才能解析
int peer::insert_node(std::string &node_info)
{
    char node_info_str[1024] = "";
    strcpy(node_info_str, node_info.c_str() + 5);
    char *node = strtok(node_info_str, ":");
    char *ip = strtok(NULL, ":");
	char *port = strtok(NULL, ":");
    std::cout << "node information is: " << node << " " << ip << " " << port << std::endl;
    unsigned short port_short = atoi(port);
    ba::ip::udp::endpoint ep(ba::ip::address::from_string(ip), port_short);
    // 如果节点存在的话就覆盖原来的endpoint，不存在的话就加入新的
    list_node_endpoint[node] = ep;
    return 0;
}

// 获取用户输入的操作命令
void get_input(peer * pr)
{
    std::string _write_message;
    int flag = 0;
    while(std::getline(std::cin, _write_message)){
        std::cout << "your input is: " << _write_message << std::endl;
        if(strncmp(_write_message.c_str(), "peer:", 5) == 0){
            char str_endpoint[127];
    		strcpy(str_endpoint, _write_message.c_str() + 5);
    		char *peer_ip = strtok(str_endpoint, ":");
    		char *peer_port = strtok(NULL, ":");
            std::cout << peer_ip << " " << peer_port << std::endl;
            pr->connect_peer(ba::ip::udp::endpoint(boost::asio::ip::address::from_string(peer_ip),std::atoi(peer_port)));
        }
    }
}

