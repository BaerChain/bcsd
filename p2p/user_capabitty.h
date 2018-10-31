#pragma 

#include <chrono>
#include <thread>
#include <libp2p/Common.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <libp2p/PeerCapability.h>
#include <libp2p/HostCapability.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace std;
using namespace dev;
using namespace dev::p2p;

class UserCapabitty :public p2p::PeerCapability
{
public:
    UserCapabitty(weak_ptr<SessionFace> _s, std::string const& _name,
        unsigned _messageCount, unsigned _idOffset, CapDesc const&)
        : PeerCapability(move(_s), _name, _messageCount, _idOffset), m_cntReceivedMessages(0), m_testSum(0)
    {}

    int countReceivedMessages() { return m_cntReceivedMessages; }
    int testSum() { return m_testSum; }
    static std::string name() { return "test"; }
    static u256 version() { return 2; }
    static unsigned messageCount() { return UserPacket + 1; }
    void sendTestMessage(int _i) { RLPStream s; sealAndSend(prep(s, UserPacket, 1) << _i); }

protected:
    bool interpretCapabilityPacket(unsigned _id, RLP const& _r) override;

    int m_cntReceivedMessages;
    int m_testSum;
};

bool UserCapabitty::interpretCapabilityPacket(unsigned _id, RLP const& _r)
{
    //cnote << "Capability::interpret(): custom message received";
    ++m_cntReceivedMessages;
    m_testSum += _r[0].toInt();
    if(_id ==p2p::PacketType::UserPacket)
		return (_id == p2p::PacketType::UserPacket);
    else
    {
        std::cout << "p2p::PacketType error!" << std::endl;
    }
}

class UserHostCapabitty : public p2p::HostCapability<UserCapabitty>, public dev::Worker
{
public:
    explicit UserHostCapabitty(Host const& _host)
        : HostCapability<UserCapabitty>(_host), Worker("test")
    {}
    virtual ~UserHostCapabitty() {}
    void sendTestMessage(NodeID const& _id, int _x)
    {
        for (auto i : peerSessions())
            if (_id == i.second->id)
                capabilityFromSession<UserCapabitty>(*i.first)->sendTestMessage(_x);
    }
    std::pair<int, int> retrieveTestData(NodeID const& _id)
    {
        int cnt = 0;
        int checksum = 0;
        for (auto i : peerSessions())
            if (_id == i.second->id)
            {
                cnt += capabilityFromSession<UserCapabitty>(*i.first)->countReceivedMessages();
                checksum += capabilityFromSession<UserCapabitty>(*i.first)->testSum();
            }

        return std::pair<int, int>(cnt, checksum);
    }
};

namespace test
{

    void test()
    {
        std::cout << "testing capabitty ..." << std::endl;

        int const step = 10;
        const char* const localhost = "127.0.0.1";
        NetworkConfig prefs1(localhost, 0, false);
        NetworkConfig prefs2(localhost, 0, false);
        p2p::Host host1("Test", prefs1);
        p2p::Host host2("Test", prefs2);

        auto thc1 = make_shared<UserHostCapabitty>(host1);
        host1.registerCapability(thc1);
        auto thc2 = make_shared<UserHostCapabitty>(host2);
        host2.registerCapability(thc2);

        host1.start();
        host2.start();

        auto port1 = host1.listenPort();
        auto port2 = host2.listenPort();

        for (unsigned i = 0; i < 3000; i += step)
        {
            this_thread::sleep_for(chrono::milliseconds(step));

            if (host1.isStarted() && host2.isStarted())
                break;
        }

        host1.requirePeer(host2.id(), NodeIPEndpoint(bi::address::from_string(localhost), port2, port2));

        for (unsigned i = 0; i < 12000; i += step)
        {
            this_thread::sleep_for(chrono::milliseconds(step));

            if ((host1.peerCount() > 0) && (host2.peerCount() > 0))
                break;
        }

        int const target = 64;
        int checksum = 0;
        for (int i = 0; i < target; checksum += i++)
            thc2->sendTestMessage(host1.id(), i);

        this_thread::sleep_for(chrono::seconds(target / 64 + 1));
        std::pair<int, int> testData = thc1->retrieveTestData(host2.id());
    }

}