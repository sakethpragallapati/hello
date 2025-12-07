#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("GoBackN");

uint32_t windowSize = 4, maxPackets = 15;
uint32_t nextSeqNum = 0, baseSeqNum = 0, packetCount = 0;

Ptr<Socket> senderSocket, receiverSocket;
std::map<uint32_t, EventId> timers;

void SendPacket(uint32_t seqNum) {
    uint8_t buf[5]; 
    buf[0] = 0; 
    memcpy(&buf[1], &seqNum, 4);

    Ptr<Packet> packet = Create<Packet>(buf, 5);
    senderSocket->Send(packet);
    NS_LOG_UNCOND("Sender: DATA Seq=" << seqNum);

    timers[seqNum] = Simulator::Schedule(Seconds(2), [seqNum]() {
        NS_LOG_UNCOND("Timeout -> Retransmit from Seq=" << baseSeqNum);
        nextSeqNum = baseSeqNum;
        for (uint32_t i = baseSeqNum; i < baseSeqNum + windowSize && i < maxPackets; i++)
            SendPacket(i);
    });
}

void SendWindow() {
    while (nextSeqNum < baseSeqNum + windowSize && nextSeqNum < maxPackets)
        SendPacket(nextSeqNum++);
}

void ReceivePacket(Ptr<Socket> socket) {
    Ptr<Packet> packet;

    while ((packet = socket->Recv())) {
        uint8_t buf[5]; 
        packet->CopyData(buf, 5);
        uint8_t type = buf[0]; 
        uint32_t num; 
        memcpy(&num, &buf[1], 4);

        if (type == 0) {
            NS_LOG_UNCOND("Receiver: DATA Seq=" << num);
            uint8_t ack[5]; ack[0] = 1; memcpy(&ack[1], &num, 4);
            receiverSocket->Send(Create<Packet>(ack, 5));
            NS_LOG_UNCOND("Receiver: ACK " << num);

            packetCount++;
        } 
        else {
            NS_LOG_UNCOND("Sender: ACK " << num);

            if (timers.count(num)) { Simulator::Cancel(timers[num]); timers.erase(num); }

            if (num >= baseSeqNum) {
                baseSeqNum = num + 1;
                if (baseSeqNum < maxPackets) SendWindow();
            }
        }
    }
}

int main() {
    NodeContainer nodes; 
    nodes.Create(2);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(0,0,0));
    nodes.Get(1)->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(100,0,0));

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper stack; 
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    auto interfaces = address.Assign(devices);

    senderSocket = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());
    senderSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
    senderSocket->Connect(InetSocketAddress(interfaces.GetAddress(1), 8080));
    senderSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    receiverSocket = Socket::CreateSocket(nodes.Get(1), UdpSocketFactory::GetTypeId());
    receiverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
    receiverSocket->Connect(InetSocketAddress(interfaces.GetAddress(0), 8080));
    receiverSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    Simulator::Schedule(Seconds(1), &SendWindow);

    AnimationInterface anim("go-back-n.xml");
    anim.SetConstantPosition(nodes.Get(0), 10, 30);
    anim.SetConstantPosition(nodes.Get(1), 90, 30);
    anim.UpdateNodeDescription(0, "Sender");
    anim.UpdateNodeDescription(1, "Receiver");
    anim.UpdateNodeColor(0, 255, 0, 0);
    anim.UpdateNodeColor(1, 0, 255, 0);
    anim.EnablePacketMetadata(true);

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_UNCOND("Simulation finished. Received " << packetCount << " packets.");
    return 0;
}