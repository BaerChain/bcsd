#include <p2p.hpp>

// 构造函数，生成接收端的socket，然后把接收的回调函数提给系统
peer::peer(ba::io_service &ios, unsigned short peer_port) 
    : io_service_con(ios)
    , tcp_ourself_endpoint(ba::ip::tcp::endpoint(ba::ip::tcp::v4(), peer_port))
    , _receive_socket(ios, ba::ip::udp::endpoint(ba::ip::udp::v4(), peer_port))
    //, server_socket(ios)
    , server_acceptor(ios, tcp_ourself_endpoint)
    //, _send_socket(ios)
{
    std::cout << peer_port << std::endl;
    leveldb_path = "./local";
    config_path = "../kv_config.json";
    leveldb_control.init_db(leveldb_path.string().c_str(), config_path.string().c_str());
    session_udp_receive();
    session_tcp_receive();
}

// 异步执行接收数据的任务
void peer::session_udp_receive()
{
    memset(receive_buf, 0, 1500);
	_receive_socket.async_receive_from(
		boost::asio::buffer(receive_buf),
		other_peer_send_endpoint,
		boost::bind(&peer::process_receive,
		this,
		ba::placeholders::error));
    
    //_send_socket.open(ba::ip::udp::v4());
}

void peer::session_tcp_receive()
{
    
    //boost::shared_ptr<ba::ip::tcp::socket> new_socket(new ba::ip::tcp::socket(io_service_con));
    ba::ip::tcp::socket * new_socket(new ba::ip::tcp::socket(io_service_con));
    server_acceptor.async_accept(
        *new_socket,
        boost::bind(&peer::tcp_process_link, 
        this, 
        new_socket,
        ba::placeholders::error));
}

void peer::tcp_process_link(ba::ip::tcp::socket * new_socket, const boost::system::error_code &ec)
{
    std::cout << "other peer port is: " << new_socket->remote_endpoint().port() << std::endl;
    std::cout << "local peer port is: " << new_socket->local_endpoint().port() << std::endl;
    boost::thread(boost::bind(&peer::tcp_process_receive, this, new_socket));
    //server_socket.close();
    //sleep(1);
    // ---***没有用智能指针，注意socket指针指向空间的释放问题***---
    session_tcp_receive();
}

void peer::tcp_process_receive(ba::ip::tcp::socket * current_socket)
{
    //std::cout << "other peer port is: " << current_socket->remote_endpoint().port() << std::endl;
    //std::cout << "local peer port is: " << current_socket->local_endpoint().port() << std::endl;
    while(1){
        content_info receive_content;
        current_socket->read_some(ba::buffer(&receive_content, sizeof(content_info)));
        if(receive_content.message_type == type_of_message){
            if(strncmp(receive_content.content, "getallkey:", 10) == 0){
                std::cout << "in get all key" << std::endl;
                std::map<string, string> all_kv;
                leveldb_control.get_all(all_kv);
                std::map<string, string>::iterator it;
                for(it = all_kv.begin(); it != all_kv.end(); it++){
                    std::cout << it->first << " " << it->second << std::endl;
                    content_info content;
                    content.content_size = it->second.length();
                    content.message_type = kv_data;
                    strcpy(content.key, it->first.c_str());
                    strncpy(content.content, it->second.c_str(), content.content_size);
                    current_socket->write_some(ba::buffer(&content, sizeof(content_info)));
                }
                transfer_tcp_string(*current_socket, "bye:");
                break;
            }
            std::cout << receive_content.content << std::endl;
        }
    }
    /*bfs::path file_path = "./4.jpg";
    bfs::fstream file;
    file.open(file_path, std::ios::binary | std::ios::in);
    int file_size = bfs::file_size(file_path);
    memset(file_buf, 0, 1024 * 1024);
    file.read(file_buf, file_size);
    current_socket->write_some(ba::buffer(&file_size, sizeof(int)));
    current_socket->write_some(ba::buffer(file_buf, file_size));*/
    //std::cout << file_buf << std::endl;
    
    
    current_socket->close();
    free(current_socket);
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
    		strcpy(str_file_path, string_temp.c_str() + 8);
            //char *cmd = strtok(str_file_path, ":");
    		//char *file_path_char = strtok(NULL, ":");
            std::string file_hash = str_file_path;
            char buf[256] = "";
            sprintf(buf, "exist:%s", str_file_path);
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
    		strcpy(str_file_path, string_temp.c_str() + 9);
            //char *cmd = strtok(str_file_path, ":");
    		//char *file_path_char = strtok(NULL, ":");
            std::cout << "file_path: " << str_file_path << std::endl;
            bfs::path file_path = str_file_path;
            string json_content;
            std::string file_hash = str_file_path;

            leveldb_control.get_message(file_hash, json_content);
            send_string_message(json_content.c_str(), other_peer_send_endpoint);
            //transfer_file(file_path, other_peer_send_endpoint);
            std::cout << "file " << str_file_path << " is transfer complete!" << std::endl;
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
    session_udp_receive();
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
    		strcpy(str_file_path, _write_message.c_str() + 5);
            //char *cmd = strtok(str_file_path, ":");
    		//char *file_path_char = strtok(NULL, ":");
            std::cout << "file_path: " << str_file_path << std::endl;
            bfs::path file_path = str_file_path;
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
    pr->keep_same_leveldb();
    //int flag = 0;
    ba::ip::tcp::socket client_socket(pr->io_service_con);
    while(std::getline(std::cin, _write_message)){
        std::cout << "your input is: " << _write_message << std::endl;
        if(strncmp(_write_message.c_str(), "peer:", 5) == 0){
            char str_endpoint[127];
    		strcpy(str_endpoint, _write_message.c_str() + 5);
    		char *peer_ip = strtok(str_endpoint, ":");
    		char *peer_port = strtok(NULL, ":");
            std::cout << peer_ip << " " << peer_port << std::endl;
            pr->connect_peer(ba::ip::udp::endpoint(boost::asio::ip::address::from_string(peer_ip),std::atoi(peer_port)));
        }else if(strncmp(_write_message.c_str(), "tcp:", 4) == 0){
            char str_endpoint[127];
    		strcpy(str_endpoint, _write_message.c_str() + 4);
    		char *peer_ip = strtok(str_endpoint, ":");
    		char *peer_port = strtok(NULL, ":");
            std::cout << peer_ip << " " << peer_port << std::endl;
            
            client_socket.connect(ba::ip::tcp::endpoint(boost::asio::ip::address::from_string(peer_ip),std::atoi(peer_port)));
            
        }else if(strncmp(_write_message.c_str(), "cut:", 4) == 0){
            client_socket.close();
        }else{
            if(client_socket.is_open()){
                client_socket.write_some(ba::buffer(_write_message.c_str(), _write_message.length()));
                char buf[1024 * 1024];
                int file_size = 0;
                client_socket.read_some(ba::buffer(&file_size, sizeof(int)));
                client_socket.read_some(ba::buffer(buf, 1024 * 1024));
                bfs::path file_path = "../100.jpg";
                bfs::fstream file;
                file.open(file_path, std::ios::binary | std::ios::out);
                file.write(buf, file_size);
                file.close();
            }
        }
    }
}

void peer::transfer_tcp_file(bfs::path file_path, ba::ip::tcp::endpoint target_endpoint)
{
    ba::ip::tcp::socket client_socket(io_service_con);
    client_socket.connect(target_endpoint);
    
}

void peer::transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message)
{
    //ba::ip::tcp::socket client_socket(io_service_con);
    //client_socket.connect(target_endpoint);
    content_info message_info;
    message_info.message_type = type_of_message;
    message_info.content_size = message.length();
    strcpy(message_info.content, message.c_str());
    client_socket.write_some(ba::buffer(&message_info, sizeof(content_info)));
}

void peer::transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message, ba::ip::tcp::endpoint &target_endpoint)
{
    client_socket.connect(target_endpoint);
    content_info message_info;
    message_info.message_type = type_of_message;
    message_info.content_size = message.length();
    strcpy(message_info.content, message.c_str());
    client_socket.write_some(ba::buffer(&message_info, sizeof(content_info)));
}

int peer::load_config(bfs::path config_path)
{
    bfs::fstream config_file;
    config_file.open(config_path, std::ios::in);
    std::string node_info;
    while(!config_file.eof()){
        getline(config_file, node_info);
        char node_info_char[128] = "";
        strcpy(node_info_char, node_info.c_str());
        char * node_id = strtok(node_info_char, ":");
        char * ip = strtok(NULL, ":");
        char * port = strtok(NULL, ":");

        if(node_id != NULL && ip != NULL && port != NULL){
            unsigned short port_short = atoi(port);
            ba::ip::udp::endpoint ep(ba::ip::address::from_string(ip), port_short);
            list_node_endpoint[node_id] = ep;
        }
        std::cout << node_info << std::endl;
    }
    return 0;
}

int peer::keep_same_leveldb()
{
    std::map<std::string, ba::ip::udp::endpoint>::iterator it;
    ba::ip::tcp::socket client_socket(io_service_con);
    for(it = list_node_endpoint.begin(); it != list_node_endpoint.end(); it++){
        ba::ip::tcp::endpoint current_point;
        udp2tcp(it->second, current_point);
        transfer_tcp_string(client_socket, "getallkey:", current_point);
        while(1){
            content_info kv;
            std::cout << "in while" << std::endl;
            client_socket.read_some(ba::buffer(&kv, sizeof(content_info)));
            std::cout << kv.key << std::endl;
            if(kv.message_type == kv_data){
                std::string json_content;
                leveldb_control.get_message(kv.key, json_content);
                if(json_content.empty()){
                    std::cout << "no key " << kv.key << std::endl;
                    //leveldb_control.
                }
            }else if(kv.message_type == type_of_message){
                if(strncmp(kv.content, "bye:", 4) == 0){
                    client_socket.close();
                    break;
                }
            }
        }
        //client_socket.close();
    }
    return 0;
}

int peer::udp2tcp(ba::ip::udp::endpoint &src, ba::ip::tcp::endpoint &des)
{
    ba::ip::tcp::endpoint tcp_endpoint(src.address(), src.port());
    des = tcp_endpoint;
    return 0;
}