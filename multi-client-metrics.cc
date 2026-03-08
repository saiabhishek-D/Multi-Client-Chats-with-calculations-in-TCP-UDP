#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <vector>
#include <sstream>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("MultiClientMetrics");

class ChatServerApp : public Application {
public:
    ChatServerApp() : m_socket(0), m_port(9000) {}
    void Setup(uint16_t port) { m_port = port; }
private:
    virtual void StartApplication(void) {
        Ptr<SocketFactory> sf = GetNode()->GetObject<SocketFactory>(UdpSocketFactory::GetTypeId());
        m_socket = sf->CreateSocket();
        m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
        m_socket->SetRecvCallback(MakeCallback(&ChatServerApp::HandleRead, this));
    }
    void HandleRead(Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from))) {
            std::vector<uint8_t> buf(packet->GetSize());
            packet->CopyData(buf.data(), packet->GetSize());
            std::string data = std::string((char*)buf.data(), packet->GetSize());

            NS_LOG_UNCOND("Server received: \"" << data << "\" from " << InetSocketAddress::ConvertFrom(from).GetIpv4());

            std::string response;
            if (data.substr(0, 5) == "CALC:") {
                std::string expr = data.substr(5);
                double a, b;
                char op;
                std::istringstream ss(expr);
                ss >> a >> op >> b;
                double result = 0;
                if (op == '+') result = a + b;
                else if (op == '-') result = a - b;
                else if (op == '*') result = a * b;
                else if (op == '/' && b != 0) result = a / b;
                std::ostringstream res;
                res << "RESULT: " << a << " " << op << " " << b << " = " << result;
                response = res.str();
            } else {
                std::ostringstream ack;
                ack << "ACK | Received at: " << Simulator::Now().GetSeconds() << "s | Size: " << packet->GetSize() << " bytes";
                response = ack.str();
            }

            Ptr<Packet> responsePkt = Create<Packet>((uint8_t*)response.c_str(), response.length());
            socket->SendTo(responsePkt, 0, from);
        }
    }
    Ptr<Socket> m_socket;
    uint16_t m_port;
};

class ChatClientApp : public Application {
public:
    ChatClientApp() : m_socket(0) {}
    void Setup(Address address, Time interval, std::string clientName) {
        m_peer = address;
        m_interval = interval;
        m_name = clientName;
    }
private:
    virtual void StartApplication(void) {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Connect(m_peer);
        m_socket->SetRecvCallback(MakeCallback(&ChatClientApp::HandleResponse, this));
        SendChat();
    }
    void SendChat() {
        std::string msg = "Hello from " + m_name;
        Ptr<Packet> p = Create<Packet>((uint8_t*)msg.c_str(), msg.length());
        m_socket->Send(p);
        Simulator::Schedule(m_interval, &ChatClientApp::SendChat, this);
    }
    void HandleResponse(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        std::vector<uint8_t> buf(packet->GetSize());
        packet->CopyData(buf.data(), packet->GetSize());
        std::string data = std::string((char*)buf.data(), packet->GetSize());
        NS_LOG_UNCOND("[" << m_name << "] Recv From Server -> " << data);
    }
    Ptr<Socket> m_socket;
    Address m_peer;
    Time m_interval;
    std::string m_name;
};

class CalcClientApp : public Application {
public:
    CalcClientApp() : m_socket(0) {}
    void Setup(Address address, Time interval, std::string clientName) {
        m_peer = address;
        m_interval = interval;
        m_name = clientName;
    }
private:
    virtual void StartApplication(void) {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Connect(m_peer);
        m_socket->SetRecvCallback(MakeCallback(&CalcClientApp::HandleResponse, this));
        SendCalc();
    }
    void SendCalc() {
        std::string msg = "CALC:10+20";
        Ptr<Packet> p = Create<Packet>((uint8_t*)msg.c_str(), msg.length());
        m_socket->Send(p);
        NS_LOG_UNCOND("[" << m_name << "] Sent calculation request: " << msg);
        Simulator::Schedule(m_interval, &CalcClientApp::SendCalc, this);
    }
    void HandleResponse(Ptr<Socket> socket) {
        Ptr<Packet> packet = socket->Recv();
        std::vector<uint8_t> buf(packet->GetSize());
        packet->CopyData(buf.data(), packet->GetSize());
        std::string data = std::string((char*)buf.data(), packet->GetSize());
        NS_LOG_UNCOND("[" << m_name << "] Recv From Server -> " << data);
    }
    Ptr<Socket> m_socket;
    Address m_peer;
    Time m_interval;
    std::string m_name;
};

int main(int argc, char *argv[]) {
    NodeContainer nodes;
    nodes.Create(3);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d10 = p2p.Install(nodes.Get(1), nodes.Get(0));
    NetDeviceContainer d20 = p2p.Install(nodes.Get(2), nodes.Get(0));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i10 = address.Assign(d10);
    address.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i20 = address.Assign(d20);

    Ptr<ChatServerApp> server = CreateObject<ChatServerApp>();
    server->Setup(9000);
    nodes.Get(0)->AddApplication(server);
    server->SetStartTime(Seconds(1.0));
    server->SetStopTime(Seconds(15.0));

    Ptr<ChatClientApp> clientA = CreateObject<ChatClientApp>();
    clientA->Setup(InetSocketAddress(i10.GetAddress(1), 9000), Seconds(3.0), "CLIENT_A");
    nodes.Get(1)->AddApplication(clientA);
    clientA->SetStartTime(Seconds(2.0));
    clientA->SetStopTime(Seconds(14.0));

    Ptr<CalcClientApp> clientB = CreateObject<CalcClientApp>();
    clientB->Setup(InetSocketAddress(i20.GetAddress(1), 9000), Seconds(5.0), "CLIENT_B");
    nodes.Get(2)->AddApplication(clientB);
    clientB->SetStartTime(Seconds(2.0));
    clientB->SetStopTime(Seconds(14.0));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_UNCOND("--- Starting Simulation ---");
    Simulator::Stop(Seconds(15.0));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    NS_LOG_UNCOND("\n--- Network Performance Measurements ---");
    for (auto& flow : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        NS_LOG_UNCOND("Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
        NS_LOG_UNCOND("  Tx Packets: " << flow.second.txPackets);
        NS_LOG_UNCOND("  Rx Packets: " << flow.second.rxPackets);
        NS_LOG_UNCOND("  Lost Packets: " << flow.second.lostPackets);
        NS_LOG_UNCOND("  Throughput: " << flow.second.rxBytes * 8.0 / 15.0 / 1000 << " Kbps");
        NS_LOG_UNCOND("  Avg Delay: " << (flow.second.rxPackets > 0 ? flow.second.delaySum.GetSeconds() / flow.second.rxPackets * 1000 : 0) << " ms");
    }

    Simulator::Destroy();
    NS_LOG_UNCOND("--- Simulation Finished ---");

    return 0;
}
