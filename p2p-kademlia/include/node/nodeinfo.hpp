//
// Created by jestjest on 11/11/17.
//

#ifndef SIMPLE_KADEMLIA_NODEINFO_HPP
#define SIMPLE_KADEMLIA_NODEINFO_HPP

#include <aux/picosha2.hpp>
#include <arpa/inet.h>
#include <boost/multiprecision/cpp_int.hpp>

namespace kdml {

    namespace mp = boost::multiprecision;

    struct NodeInfo {

        uint32_t ipAddr{};
        unsigned short port{0};
        mp::uint256_t id{};

        NodeInfo() = default;

        NodeInfo(const std::string& ip, unsigned short p)
                : port(p) {

            struct sockaddr_in sa;
            inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
            ipAddr = sa.sin_addr.s_addr;

            std::string ipPort = ip + std::to_string(p);
            std::vector<unsigned char> hash(32);
            picosha2::hash256(ipPort.begin(), ipPort.end(), hash.begin(), hash.end());
            mp::import_bits(id, hash.begin(), hash.end());
        }

        template<class Archive>
        void save(Archive& ar) const {
            ar(ipAddr, port);
        }

        template<class Archive>
        void load(Archive& ar) {
            ar(ipAddr, port);
            std::string ip = getIpAddr() + std::to_string(port);
            std::vector<unsigned char> hash(32);
            picosha2::hash256(ip.begin(), ip.end(), hash.begin(), hash.end());
            mp::import_bits(id, hash.begin(), hash.end());
        }

        friend std::ostream& operator<<(std::ostream& os, const NodeInfo& node) {
            os << "(id: " << node.id
               << ", IP: " << node.getIpAddr()
               << ":" << node.port << ")";
            return os;
        }

        std::string getIpAddr() const {
            char str[INET_ADDRSTRLEN];
            struct sockaddr_in sa;
            sa.sin_addr.s_addr = ipAddr;
            inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);
            std::string addrStr(str);
            return addrStr;
        }
    };
}

#endif //SIMPLE_KADEMLIA_NODEINFO_HPP
