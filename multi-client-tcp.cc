#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("MultiClientTCP");

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

    uint16_t port = 9000;

    // SERVER - PacketSink for TCP
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(0));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(15.0));

    // CLIENT A - sends every 3 seconds
    OnOffHelper clientA("ns3::TcpSocketFactory",
        InetSocketAddress(i10.GetAddress(1), port));
    clientA.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientA.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=2]"));
    clientA.SetAttribute("DataRate", StringValue("512Kbps"));
    clientA.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer appA = clientA.Install(nodes.Get(1));
    appA.Start(Seconds(2.0));
    appA.Stop(Seconds(14.0));

    // CLIENT B - sends every 5 seconds
    OnOffHelper clientB("ns3::TcpSocketFactory",
        InetSocketAddress(i20.GetAddress(1), port));
    clientB.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientB.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=4]"));
    clientB.SetAttribute("DataRate", StringValue("512Kbps"));
    clientB.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer appB = clientB.Install(nodes.Get(2));
    appB.Start(Seconds(2.0));
    appB.Stop(Seconds(14.0));

    NS_LOG_UNCOND("--- Starting TCP Multi Client Simulation ---");
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    Simulator::Stop(Seconds(15.0));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("--- Simulation Finished ---");

    return 0;
}
