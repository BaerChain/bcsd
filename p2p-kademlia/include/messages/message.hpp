//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_MESSAGE_HPP
#define SIMPLE_KADEMLIA_MESSAGE_HPP

#include "messageType.hpp"
#include <cereal/types/memory.hpp>
#include <boost/multiprecision/cpp_int.hpp>


namespace kdml {
    namespace net {

        namespace mp = boost::multiprecision;

        class Message {
        protected:
            uint32_t tid;
            MessageType mtype{};
        public:
            explicit Message(uint32_t tid, MessageType type) : tid(tid), mtype(type) {}

            MessageType getMessageType() const { return mtype; }
            uint32_t getTid() const { return tid; }

            virtual void print(std::ostream&) const = 0;

            friend std::ostream& operator<<(std::ostream& stream, Message& msg) {
                msg.print(stream);
                return stream;
            }

            template <class Archive>
            void serialize(Archive& ar) {
                ar(tid, mtype);
            }
        };
    }
}

#endif //SIMPLE_KADEMLIA_MESSAGE_HPP
