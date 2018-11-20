#include <p2p.hpp>
#include <boost/algorithm/string/split.hpp>
#include <plugin_check.hpp>
#include <exception>

// 构造函数，生成接收端的socket，然后把接收的回调函数提给系统
peer::peer(ba::io_service &ios, unsigned short peer_port) 
    : io_service_con(ios)
    , tcp_ourself_endpoint(ba::ip::tcp::endpoint(ba::ip::tcp::v4(), peer_port))
    , ourself_endpoint(ba::ip::udp::endpoint(ba::ip::udp::v4(), peer_port))
    , _receive_socket(ios, ourself_endpoint)
    //, server_socket(ios)
    , server_acceptor(ios, tcp_ourself_endpoint)
    //, _send_socket(ios)
{
    std::cout << peer_port << std::endl;
    leveldb_path = "./local";
    config_path = "../kv_config.json";

	leveldb_control = CFirstLevelDb::get_single_level_db();
	if(!leveldb_control->is_open())
		leveldb_control->init_db(leveldb_path.string().c_str(), config_path.string().c_str());
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
        int re = ba::read(*current_socket, ba::buffer(&receive_content, sizeof(content_info)));
        //current_socket->read_some(ba::buffer(&receive_content, sizeof(content_info)));
        std::cout << "receive size is " << re << " content is " << receive_content.content << std::endl;
        if(receive_content.message_type == type_of_message){
            if(strncmp(receive_content.content, "getallkey:", 10) == 0){
                std::cout << "in get all key" << std::endl;
                std::map<string, string> all_kv;
                leveldb_control->get_all(all_kv);
                std::map<string, string>::iterator it;
                int index = 1;
                for(it = all_kv.begin(); it != all_kv.end(); it++){
                    std::cout << "index " << index << " " << std::endl;
                    std::cout << it->first << " " << it->second << std::endl;
                    content_info content;
                    content.content_size = it->second.length();
                    content.message_type = kv_data;
                    strcpy(content.key, it->first.c_str());
                    strncpy(content.content, it->second.c_str(), content.content_size);
                    int re = current_socket->write_some(ba::buffer(&content, sizeof(content_info)));
                    std::cout << "write some re is " << re << std::endl;
                }
                transfer_tcp_string(*current_socket, "bye:");
                break;
            }else if(strncmp(receive_content.content, "getblock:", 9) == 0){
                char file_hash[65] = "";
                char path_char[80] = "";
                strncpy(file_hash, receive_content.content + 9, 64);
                bfs::path block_path = store_path;
                block_path /= "L2";
                tools::sha_to_path(file_hash, path_char);
                block_path /= path_char;
                block_path /= file_hash;

                std::cout << "file path is " << block_path << std::endl;
                if(bfs::exists(block_path)){
                    std::cout << "-----file is exist, path is " << block_path << std::endl;
                    transfer_tcp_file(*current_socket, block_path);
                }
                
            }else if(strncmp(receive_content.content, "getkey:", 7) == 0){  // 获取某个key的value
                std::string file_hash(receive_content.content + 7, receive_content.content_size - 7);
                std::string hash_value;
                std::cout << "receive file hash is " << file_hash << std::endl;
                leveldb_control.get_message(file_hash, hash_value);
                if(!hash_value.empty()){
                    // 存在的话就直接返回
                    transfer_tcp_string(*current_socket, hash_value, kv_data);
                }else{
                    transfer_tcp_string(*current_socket, "bye:");
                }
                break;
            }else if(strncmp(receive_content.content, "bye:", 4) == 0){
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

void peer::tcp_get_value_and_block(std::string file_hash, ba::ip::tcp::endpoint other_peer_server_endpoint)
{
    std::cout << "in tcp_get_value_and_block file_hash is " << file_hash << " endpoint is " << other_peer_server_endpoint << std::endl;
    ba::ip::tcp::socket current_socket(io_service_con);
    current_socket.connect(other_peer_server_endpoint);
    char get_value_of_file_hash[128] = "";
    sprintf(get_value_of_file_hash, "getkey:%s", file_hash.c_str());
    transfer_tcp_string(current_socket, get_value_of_file_hash);
    content_info receive_buf;
    ba::read(current_socket, ba::buffer(&receive_buf, sizeof(content_info)));
    if(receive_buf.message_type == kv_data){
        get_file_in_key(receive_buf.content, other_peer_server_endpoint);
        // 本机同步数据后，也该给自己的节点表里的节点依次发送增加文件的消息
        // to do！
    }else if(receive_buf.message_type == type_of_message){
        if(strncmp(receive_buf.content, "bye:", 4) == 0){
            current_socket.close();
        }
    }
    
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
            leveldb_control->get_message(file_hash, json_content);
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
        }
		else if (mb.message_type == check)
		{
			//挑战
			std::string order_str= std::string(mb.value);
			bpo::variables_map options;
			//得到 hash
			// _order = "check_file:" + check_file + "|" + "offset:" + offset + "|" + "length:" + length;
			cout << "get mesage order : " << order_str << endl;
			vector<string> vStr;
			tools::SplitString(order_str, vStr, "|");
			std::string check_file;
			std::string offset;
			std::string length;
			for (int i=0; i<vStr.size(); i++)
			{
				if (vStr[i].find("check_file"))
					check_file = vStr[i].substr(strlen("check_file")+1, vStr[i].length());
				else if (vStr[i].find("offset"))
					offset = vStr[i].substr(strlen("offset")+1, vStr[i].length());
				else if (vStr[i].find("length"))
					length = vStr[i].substr(strlen("offset")+1, vStr[i].length());
			}
			string hash_str;
			auto* m_check = new plugin_check();
			m_check->initialize(options);
			m_check->set_optins(check_file, offset, length);
			hash_str = m_check->get_offset_hash();
			delete m_check;
			send_string_message(hash_str.c_str(), other_peer_send_endpoint);
			
		}
		else if(strncmp(string_temp.c_str(), "transfer:", 9) == 0){    // 收到这个消息头表示正式开始传文件
            char str_file_path[127];
    		strcpy(str_file_path, string_temp.c_str() + 9);
            //char *cmd = strtok(str_file_path, ":");
    		//char *file_path_char = strtok(NULL, ":");
            std::cout << "file_path: " << str_file_path << std::endl;
            bfs::path file_path = str_file_path;
            string json_content;
            std::string file_hash = str_file_path;

            leveldb_control->get_message(file_hash, json_content);
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
        }else if(strncmp(string_temp.c_str(), "addfile:", 8) == 0){   // 收到对方添加文件的消息
            std::string json_content;
            // 检查当前结点是否含有此文件，没有就去请求对方发送给自己
            std::string file_hash(string_temp.c_str() + 8, string_temp.length() - 8);
            std::cout << "file hash is " << file_hash << std::endl;
            leveldb_control.get_message(file_hash, json_content);
            if(json_content.empty()){
                std::cout << "current key is not exist!" << std::endl;
                ba::ip::tcp::endpoint other_tcp_server_endpoint;
                udp2tcp(other_peer_send_endpoint, other_tcp_server_endpoint);
                boost::thread(boost::bind(&peer::tcp_get_value_and_block, this, file_hash, other_tcp_server_endpoint));
            }
        }
    }else{ //如果传的是文件 暂时udp传文件还没完善，后续传文件不用udp这套
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
    if(ep != ourself_endpoint){
        // 如果节点存在的话就覆盖原来的endpoint，不存在的话就加入新的
        list_node_endpoint[node] = ep;
    }
    
    return 0;
}

// 获取用户输入的操作命令
void get_input(peer * pr)
{
    std::string _write_message;
    pr->keep_same_leveldb();
    //int flag = 0;
    ba::ip::tcp::socket client_socket(pr->io_service_con);
	std::cout << " please commend to start some mode:\n" \
		"--challenge							-- start file challenge service\n" \
		"\n" << std::endl;
    while(std::getline(std::cin, _write_message))
	{
        std::cout << "your input is: \n" << _write_message << std::endl;
		if(strncmp(_write_message.c_str(), "--challenge", 5) == 0)
		{
			std::cout << " input quit to out this mode ... \n" \
				"please paramers to challenge eg:\n" \
				"-n=(string)_node -c=(string)_check_file -f=(int)_offset -l=(int)_length \n"<< std::endl;
			
			while (true)
			{
				std::string nodeid;
				std::string check_file;
				std::string offset;
				std::string length;
				std::string _order;

				string temp_order;
				std::getline(std::cin, temp_order);
				if (temp_order.find("quit") == 0)
				{
					break;
				}
				vector<string> vStr;
				tools::SplitString(temp_order, vStr, " ");
				for (int i = 0; i < vStr.size(); i++)
				{
					int op_size = vStr[i].length();
					if (std::string::npos != vStr[i].find("-c"))
						check_file = vStr[i].substr(strlen("-c") + 1, op_size);
					else if (std::string::npos != vStr[i].find("-f"))
						offset = vStr[i].substr(strlen("-f") + 1, op_size);
					else if (std::string::npos != vStr[i].find("-l"))
						length = vStr[i].substr(strlen("-l") + 1, op_size);
					else if (std::string::npos != vStr[i].find("-n"))
						nodeid = vStr[i].substr(strlen("-n") + 1, op_size);
				}
				if (check_file.empty() || offset.empty() || length.empty() || nodeid.empty())
				{
					cout << " input the commend  is error ... please input twice" << endl;
					continue;
				}
			
				_order = "check_file:" + check_file + "|" + "offset:" + offset + "|" + "length:" + length;
				std::cout << "order:[" << _order << "]" << std::endl;
				
				std::string result_str = pr->challenge(client_socket, nodeid, _order);
				std::cout << "result_str:[" << result_str <<"]" << std::endl;

				//校验
				bpo::variables_map options;
				auto* m_check = new plugin_check();
				m_check->initialize(options);
				m_check->set_optins(check_file, offset, length);
				std::string result_owner = m_check->get_offset_hash();

				std::cout << "result_owner:[" << result_owner << "]" << std::endl;
				delete m_check;
			}

		}
		else if(strncmp(_write_message.c_str(), "peer:", 5) == 0){
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
        }else if(strncmp(_write_message.c_str(), "out:", 4) == 0){
            break;
        }else if(strncmp(_write_message.c_str(), "add_file ", 9) == 0){
            plugin_add add_file_contraler;
            char node_info_char[256] = "";
            strcpy(node_info_char, _write_message.c_str());
            char * cmd = strtok(node_info_char, " ");
            char * file = strtok(NULL, " ");
        	cmd = strtok(NULL, " ");
        	char * game_name = strtok(NULL, " ");
        	cmd = strtok(NULL, " ");
        	char * game_version = strtok(NULL, " ");
            std::cout << cmd << " " << file << " " << game_name << " " << game_version << std::endl;
            //add_file_contraler.set_initialize(file, game_name, game_version);
            // 本地添加成功后，给自己本地存储的节点依次发送添加文件的命令
            
            //char cmd_add_file[128] = "";
            //sprintf(cmd_add_file, "addfile:%s", add_file_contraler.get_file_hash().c_str());

            char cmd_add_file[128] = "addfile:aceb111";
            pr->udp_send_in_order(cmd_add_file);
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
		_write_message.clear();
    }
}

int peer::udp_send_in_order(std::string message)
{
    std::map<std::string, ba::ip::udp::endpoint>::iterator it;
    for(it = list_node_endpoint.begin(); it != list_node_endpoint.end(); it++){
        send_string_message(message.c_str(), it->second);
    }
    return 0;
}

void peer::transfer_tcp_file(ba::ip::tcp::socket & client_socket, bfs::path file_path)
{
    //ba::ip::tcp::socket client_socket(io_service_con);
    //client_socket.connect(target_endpoint);
    bfs::fstream file;
    content_info send_content;
    file.open(file_path, std::ios::binary | std::ios::in);
    send_content.content_size = bfs::file_size(file_path);
    send_content.message_type = type_of_file;
    file.read(send_content.content, send_content.content_size);
    client_socket.write_some(ba::buffer(&send_content, sizeof(content_info)));
    file.close();
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

void peer::transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message, enum_message_type type)
{
    //ba::ip::tcp::socket client_socket(io_service_con);
    //client_socket.connect(target_endpoint);
    content_info message_info;
    message_info.message_type = type;
    message_info.content_size = message.length();
    strcpy(message_info.content, message.c_str());
    client_socket.write_some(ba::buffer(&message_info, sizeof(content_info)));
}

void peer::transfer_tcp_string(ba::ip::tcp::socket & client_socket, std::string message, ba::ip::tcp::endpoint &target_endpoint)
{
    boost::system::error_code error;
    client_socket.connect(target_endpoint, error);
    if(error){
        std::cout << error.message() << std::endl;
        throw 1;
    }
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
        printf("node_id:%p, ip:%p, port:%p", node_id, ip, port);
        if(node_id != NULL && ip != NULL && port != NULL){
            std::cout << "inset node in to list" << std::endl;
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
        send_string_message("getallnode:", it->second); // 同步公共点的节点表
        // 发送自己的信息给对方
        char buf[1024] = "";
        sprintf(buf, "node:%s:%s:%u", node_id.c_str(), _receive_socket.local_endpoint().address().to_string().c_str(), _receive_socket.local_endpoint().port());
        std::cout << "buf is:" << buf << std::endl;
        send_string_message(buf, it->second); 
        
        // 如果本地表里的endpoint连接不上，就跳到下一个，并显示，后续可以删除该条记录
        try{
            transfer_tcp_string(client_socket, "getallkey:", current_point);
        }catch(int error_number){
            if(1 == error_number){
                std::cout << "connect to " << current_point << " is failed!" << std::endl;
                continue;
            }
        }
        while(1){
            content_info kv;
            std::cout << "in while" << std::endl;
            int re = ba::read(client_socket, ba::buffer(&kv, sizeof(content_info)));
            //int re = client_socket.read(ba::buffer(&kv, sizeof(content_info)));
            std::cout << "read_some re is " << re << " sizeof content_info is " << sizeof(content_info) << std::endl;
            std::cout << "key:" << kv.key << std::endl;
            std::cout << "value:" << kv.content << std::endl;
            
            if(kv.message_type == kv_data){
                std::string json_content;
                leveldb_control->get_message(kv.key, json_content);
                if(json_content.empty()){
                    std::cout << "no key " << kv.key << std::endl;
                    leveldb_control->insert_key_value(kv.key, kv.content);
                    get_file_in_key(kv.content, current_point);
                }else{
                    continue;
                }
            }else if(kv.message_type == type_of_message){
                if(strncmp(kv.content, "bye:", 4) == 0){
                    client_socket.close();
                    break;
                }else{
                    client_socket.close();
                    break;
                }
            }
        }
        //client_socket.close();
    }
    return 0;
}

int peer::get_file_in_key(std::string root_json, ba::ip::tcp::endpoint current_point)
{
    ba::ip::tcp::socket client_socket(io_service_con);
    client_socket.connect(current_point);
    Json::Reader root_reader;
    Json::Value node;
    root_reader.parse(root_json, node);
    Json::Value block_array = node["block"];
    unsigned int i = 0;
    for(; i < block_array.size(); i++) {
        content_info block_info;
        std::string getblock = "getblock:";
        getblock += block_array[i]["value"].asString();
        std::cout << getblock << std::endl;
        transfer_tcp_string(client_socket, getblock);
        ba::read(client_socket, ba::buffer(&block_info, sizeof(content_info)));
        
        char file_hash[65] = "";
        char path_char[80] = "";
        strncpy(file_hash, block_array[i]["value"].asString().c_str(), 64);
        bfs::path block_path = store_path;
        block_path /= "L2";
        tools::sha_to_path(file_hash, path_char);
        block_path /= path_char;
        block_path /= file_hash;

        bfs::fstream block_file;
        block_file.open(block_path, std::ios::binary | std::ios::out);
        block_file.write(block_info.content, block_info.content_size);
        block_file.close();
    }
    transfer_tcp_string(client_socket, "bye:");
    client_socket.close();
    return 0;
}
std::string  peer::challenge(ba::ip::tcp::socket& _socket, ba::ip::tcp::endpoint &target_endpoint, const std::string& _order)
{
    std::cout << "in challenge" << std::endl;
	if (!_socket.is_open())
	{
		//_socket = ba::ip::tcp::socket(io_service_con);
		_socket.connect(target_endpoint);
	}
	message_block message_info;
	message_info.message_type = check;
	message_info.size = _order.length();
	strcpy(message_info.value, _order.c_str());
    std::cout << "will be write" << std::endl;
	_socket.write_some(ba::buffer(&message_info, sizeof(content_info)));
	
	std::string result_str;
	while (true)
	{
		//等待接收
		content_info _data;
		_socket.read_some(ba::buffer(&_data, sizeof(content_info)));
		std::cout << _data.content << std::endl;

		result_str =std::string(_data.content);
		if (_data.message_type == check)
			break;
	}
	return result_str;
}

std::string peer::challenge(ba::ip::tcp::socket& _socket, const std::string _nodeid, const std::string _order)
{
	if (list_node_endpoint.find(_nodeid) == list_node_endpoint.end())
	{
		std::cout << "cant't find node_id[" << _nodeid << "]" << std::endl;
		return std::string();
	}
	ba::ip::tcp::endpoint current_point;
	udp2tcp(list_node_endpoint[_nodeid], current_point);
	return challenge(_socket, current_point, _order);
}

int peer::udp2tcp(ba::ip::udp::endpoint &src, ba::ip::tcp::endpoint &des)
{
    ba::ip::tcp::endpoint tcp_endpoint(src.address(), src.port());
    des = tcp_endpoint;
    return 0;
}
int tcp2udp(ba::ip::tcp::endpoint &src, ba::ip::udp::endpoint &des)
{
    ba::ip::udp::endpoint udp_endpoint(src.address(), src.port());
    des = udp_endpoint;
    return 0;
}