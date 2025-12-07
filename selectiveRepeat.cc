#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SelectiveRepeat");

uint32_t windowSize = 4;
uint32_t maxPackets = 15;

uint32_t baseSeq = 0;
uint32_t nextSeq = 0;

Ptr<Socket> senderSocket;
Ptr<Socket> receiverSocket;

std::map<uint32_t, bool> receivedBuffer;
std::map<uint32_t, bool> ackReceived;
std::map<uint32_t, EventId> timers;

void SendPacket(uint32_t seqNum) {
    uint8_t buffer[5];
    buffer[0] = 0;
    memcpy(&buffer[1], &seqNum, sizeof(seqNum));

    Ptr<Packet> packet = Create<Packet>(buffer, 5);
    senderSocket->Send(packet);
    NS_LOG_UNCOND("Sender: Sent DATA Seq=" << seqNum << " at " << Simulator::Now().GetSeconds() << "s");
    
    timers[seqNum] = Simulator::Schedule(Seconds(2.0), [seqNum]() {
        NS_LOG_UNCOND("Timeout: Retransmitting Seq=" << seqNum);
        SendPacket(seqNum);
    });
}

void SendWindow() {
    while (nextSeq < baseSeq + windowSize && nextSeq < maxPackets) {
        SendPacket(nextSeq);
        nextSeq++;
    }
}

void ReceivePacket(Ptr<Socket> socket) {
    Ptr<Packet> pkt;
    while ((pkt = socket->Recv())) {
        uint8_t buffer[5];
        pkt->CopyData(buffer, 5);
        uint8_t type = buffer[0];
        uint32_t seqNum;
        memcpy(&seqNum, &buffer[1], sizeof(seqNum));

        if (type == 0) {
            NS_LOG_UNCOND("Receiver: Got DATA Seq=" << seqNum);
            receivedBuffer[seqNum] = true;
            uint8_t ackBuf[5];
            ackBuf[0] = 1;
            memcpy(&ackBuf[1], &seqNum, sizeof(seqNum));
            Ptr<Packet> ack = Create<Packet>(ackBuf, 5);
            receiverSocket->Send(ack);
            NS_LOG_UNCOND("Receiver: Sent ACK " << seqNum);
        }
        else if (type == 1) {
            NS_LOG_UNCOND("Sender: Received ACK for Seq=" << seqNum);
            ackReceived[seqNum] = true;
            if (timers.find(seqNum) != timers.end()) {
                Simulator::Cancel(timers[seqNum]);
                timers.erase(seqNum);
            }
            if (seqNum == baseSeq) {
                while (ackReceived[baseSeq] && baseSeq < maxPackets) {
                    baseSeq++;
                }
                SendWindow();
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
    nodes.Get(0)->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(10, 20, 0));
    nodes.Get(1)->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(90, 20, 0));

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    senderSocket = Socket::CreateSocket(nodes.Get(0), UdpSocketFactory::GetTypeId());
    senderSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
    senderSocket->Connect(InetSocketAddress(interfaces.GetAddress(1), 8080));
    senderSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    receiverSocket = Socket::CreateSocket(nodes.Get(1), UdpSocketFactory::GetTypeId());
    receiverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));
    receiverSocket->Connect(InetSocketAddress(interfaces.GetAddress(0), 8080));
    receiverSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    Simulator::Schedule(Seconds(1.0), &SendWindow);

    AnimationInterface anim("selective-repeat.xml");
    anim.SetConstantPosition(nodes.Get(0), 10, 20);
    anim.SetConstantPosition(nodes.Get(1), 90, 20);
    anim.UpdateNodeDescription(0, "Sender");
    anim.UpdateNodeDescription(1, "Receiver");
    anim.UpdateNodeColor(0, 255, 0, 0);
    anim.UpdateNodeColor(1, 0, 0, 255);
    anim.EnablePacketMetadata(true);

    Simulator::Stop(Seconds(30));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}