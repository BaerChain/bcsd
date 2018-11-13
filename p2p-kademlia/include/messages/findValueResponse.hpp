//
// Created by jestjest on 11/19/17.
//

#ifndef SIMPLE_KADEMLIA_FINDVALUERESPONSE_HPP
#define SIMPLE_KADEMLIA_FINDVALUERESPONSE_HPP

#include "responseMessage.hpp"
#include <cereal/archives/binary.hpp>
#include <callbacks.hpp>

namespace kdml {
    namespace net {

        class FindValueResponse : public ResponseMessage {

        public:
            bool found{};
            Nodes data;

            FindValueResponse(mp::uint256_t id, uint32_t tid, bool found, Nodes nodes)
                    : ResponseMessage(std::move(id), tid, QueryType::FIND_VALUE),
                      found(found), data(std::move(nodes)) {}

            void print(std::ostream& os) const override {
                os << "[" << mtype << "][" << qtype << "][TID="
                   << tid << "][nodeID=" << id << "][found=" << found
                   <<"][data=";

                for (size_t i = 0; i < data.size(); i++) {
                    os << data[i];
                    if (i != data.size() - 1) {
                        os << ", ";
                    }
                }

                os << "]";
            }

            template<class Archive>
            void serialize(Archive& ar) {
                ar(cereal::base_class<ResponseMessage>(this), found, data);
            }

            template<class Archive>
            static void load_and_construct(Archive& ar, cereal::construct<FindValueResponse>& construct) {
                uint32_t tid{};
                MessageType mtype{};
                QueryType qtype{};
                std::vector<unsigned char> idVec(32);
                mp::uint256_t id;
                bool found{};
                Nodes nodes;

                ar(tid, mtype, qtype, idVec, found, nodes);
                mp::import_bits(id, idVec.begin(), idVec.end());
                construct(id, tid, found, nodes);
            }
        };
    }
}

namespace cereal {
    template <class Archive>
    struct specialize<Archive, kdml::net::FindValueResponse, cereal::specialization::member_serialize> {};
    // cereal no longer has any ambiguity when serializing
}

CEREAL_REGISTER_TYPE(kdml::net::FindValueResponse);

#endif //SIMPLE_KADEMLIA_FINDVALUERESPONSE_HPP
