//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_MESSAGETYPE_HPP
#define SIMPLE_KADEMLIA_MESSAGETYPE_HPP

#include <iostream>

namespace kdml {
    namespace net {
        enum class MessageType {
            QUERY,
            RESPONSE,
            ERROR,
        };

        enum class QueryType {
            PING,
            FIND_NODE,
            FIND_VALUE,
            STORE,
        };

        inline std::ostream& operator<<(std::ostream& os, const MessageType value){
            const char* s = 0;
#define PROCESS_VAL(p) case(p): s = #p; break;
            switch(value){
                PROCESS_VAL(MessageType::QUERY);
                PROCESS_VAL(MessageType::RESPONSE);
                PROCESS_VAL(MessageType::ERROR);
            }
#undef PROCESS_VAL

            return os << s;
        }

        inline std::ostream& operator<<(std::ostream& os, const QueryType value){
            const char* s = 0;
#define PROCESS_VAL(p) case(p): s = #p; break;
            switch(value){
                PROCESS_VAL(QueryType::PING);
                PROCESS_VAL(QueryType::FIND_NODE);
                PROCESS_VAL(QueryType::FIND_VALUE);
                PROCESS_VAL(QueryType::STORE);
            }
#undef PROCESS_VAL

            return os << s;
        }
    }
}


#endif //SIMPLE_KADEMLIA_MESSAGETYPE_HPP
