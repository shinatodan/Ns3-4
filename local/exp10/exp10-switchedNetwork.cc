/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- 
 * exp10-switchedNetwork.cc: Simulate a switched network environment
 * F.Qian, Nov. 2012 <fei@kanto-gakuin.ac.jp>
 *                    R2
 *                p2p | 10.1.3.0/24
 *         +--------- R1 ---------+
 *         |                      |
 *      switch1                switch2
 *    +--------+             +--------+
 *    |  csma  |             |  csma  |
 *    n0  ...  nk            m0  ...  mk
 *    10.1.1.0/24            10.1.2.0/24
 */

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

#define SIM_START 0.1
#define SIM_STOP  20.1
#define PORT      9    // Discard port (RFC 863)

NS_LOG_COMPONENT_DEFINE ("exp10-switchedNet");

class MyApp : public Application 
{
  public:
	MyApp ();
	virtual ~MyApp();
	void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate);

  private:
	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void ScheduleTx (void);
	void SendPacket (void);

	Ptr<Socket>     m_socket;
	Address         m_peer;
	uint32_t        m_packetSize;
	DataRate        m_dataRate;
	EventId         m_sendEvent;
	bool            m_running;
	uint32_t        m_packetsSent;

};

MyApp::MyApp ()
	: m_socket (0), 
	  m_peer (), 
	  m_packetSize (0), 
	  m_dataRate (0), 
	  m_sendEvent (), 
	  m_running (false), 
	  m_packetsSent (0)
{
}

MyApp::~MyApp()
{
	m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind ();
	m_socket->Connect (m_peer);
	SendPacket ();
}

void 
MyApp::StopApplication (void)
{
	m_running = false;

	if (m_sendEvent.IsRunning ())
		Simulator::Cancel (m_sendEvent);
	if (m_socket)
		m_socket->Close ();
}

void 
MyApp::SendPacket (void)
{
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
	m_socket->Send (packet);
	++m_packetsSent;
	ScheduleTx ();
}

void 
MyApp::ScheduleTx (void)
{
	if (m_running) {
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
	}
}

void PrintRoutingTable (Ptr<Node>& n) {
        Ptr<Ipv4StaticRouting>  routing = 0;
        Ipv4StaticRoutingHelper routingHelper;
        Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
        uint32_t  nbRoutes = 0;
        Ipv4RoutingTableEntry route;

        routing = routingHelper.GetStaticRouting (ipv4);

        nbRoutes = routing->GetNRoutes ();

        NS_LOG_UNCOND ("Routing table of " << n << " :");
        NS_LOG_UNCOND ("----------------------------------------------------");
        std::cout << "Destination\t" << "Network mask\t" << "Gateway \t" << "I/F\n";

        for (uint32_t i = 0; i < nbRoutes; i++) {
                route = routing->GetRoute (i);
                std::cout << route.GetDestNetwork () << "\t"
                          << route.GetDestNetworkMask () << "\t"
                          << route.GetGateway () << "\t\t "
                          << route.GetInterface () << std::endl;
        }
} 

struct ADDRESS {
	std::string addr;
	std::string mask;
};
typedef struct ADDRESS NetworkAddress;

// Configure a csma switch device with nodeList 
void SwitchConfig(Ptr<Node> sw, NodeContainer nodeList, NetworkAddress net)
{
	NS_LOG_UNCOND ("Build switch Topology");
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

	NetDeviceContainer hostDevs, swDev;
	for (uint32_t i = 0; i < nodeList.GetN(); i++) {
		// install a csma channel between the ith host node and the switch node
		NetDeviceContainer link = csma.Install (NodeContainer (nodeList.Get (i), sw));
		hostDevs.Add (link.Get (0));
		swDev.Add (link.Get (1));
	}

	BridgeHelper switchHelper;
	switchHelper.SetDeviceAttribute  ("Mtu"           , UintegerValue (1500));
	switchHelper.SetDeviceAttribute  ("EnableLearning", BooleanValue (true));
	switchHelper.SetDeviceAttribute  ("ExpirationTime", TimeValue (Seconds (300)));
	switchHelper.Install (sw, swDev);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase (net.addr.c_str(), net.mask.c_str());
	ipv4.Assign (hostDevs);
}

// Configure a WAN link between nodes (n, m)
void
PointToPointConfig(Ptr<Node> n, Ptr<Node> m, NetworkAddress net)
{
	NodeContainer link = NodeContainer (n, m);
	PointToPointHelper p2p;
        p2p.SetDeviceAttribute  ("DataRate", StringValue ("100Mbps"));
        p2p.SetChannelAttribute ("Delay"   , StringValue ("5ms"));
        p2p.SetQueue ("ns3::DropTailQueue" , "MaxPackets", UintegerValue (100));
	NetDeviceContainer dev = p2p.Install (link);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase (net.addr.c_str(), net.mask.c_str());
        Ipv4InterfaceContainer ifs = ipv4.Assign (dev);
}

void
setMyTcpApp(Ptr <Node> n, Ptr <Node> m)
{
	// get node m's IP address as remote site address
        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        Address remoteAddr (InetSocketAddress (madr.GetLocal(), PORT));

	Ptr<Socket> socket = Socket::CreateSocket (n, TcpSocketFactory::GetTypeId ());
	Ptr<MyApp> app = CreateObject<MyApp> ();
	app->Setup (socket, remoteAddr, 1040, DataRate ("1Mbps"));
	n->AddApplication (app);
        app->SetStartTime (Seconds (SIM_START));
        app->SetStopTime  (Seconds (SIM_STOP ));

	// Create an optional packet sink to receive these packets
	PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
		Address (InetSocketAddress (Ipv4Address::GetAny (), PORT)));
	ApplicationContainer sink = sinkHelper.Install (m);
	sink.Start (Seconds (SIM_START));
	sink.Stop  (Seconds (SIM_STOP ));
}

void
setUdpOnOffApp(Ptr <Node> n, Ptr <Node> m)
{
        Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue ("1024"));
        Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("512kbps"));

        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        AddressValue remoteAddr (InetSocketAddress (madr.GetLocal(), PORT));
        Ipv4InterfaceAddress nadr = n->GetObject <Ipv4> ()->GetAddress(1, 0);
        Address srcAddr (InetSocketAddress (nadr.GetLocal(), PORT));

        OnOffHelper onoff  ("ns3::UdpSocketFactory", srcAddr);
        onoff.SetAttribute ("OnTime"               , StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime"              , StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("Remote"               , remoteAddr);
        ApplicationContainer onoffapps = onoff.Install (n);
        onoffapps.Start (Seconds (SIM_START));
        onoffapps.Stop  (Seconds (SIM_STOP ));
}

int 
main (int argc, char *argv[])
{
	CommandLine cmd;
	cmd.Parse (argc, argv);
	uint32_t nLan1 = 4, nLan2 = 4;

	NodeContainer routerNodes, lan1, lan2;
	Ptr<Node> R1 = CreateObject<Node> (); // Router between two switches
	Ptr<Node> R2 = CreateObject<Node> (); // Router between in/out-side networks

        Ptr<Node> *n = new Ptr<Node> [nLan1]; // create nodes on lan1(R1,n1,n2,...)
        lan1.Add (R1);
        for(uint32_t  i=0; i<nLan1; i++) {
                n[i] = CreateObject<Node> ();
                lan1.Add (n[i]);
		routerNodes.Add (n[i]);
        }

	routerNodes.Add (R1);

        Ptr<Node> *m = new Ptr<Node> [nLan2]; // create nodes on lan2(R1,m1,m2,...)
        lan2.Add (R1);
        for(uint32_t  i=0; i<nLan2; i++) {
                m[i] = CreateObject<Node> ();
                lan2.Add (m[i]);
		routerNodes.Add (m[i]);
        }

	routerNodes.Add (R2);

	Ptr<Node> switch1 = CreateObject<Node> ();
	Ptr<Node> switch2 = CreateObject<Node> ();

	InternetStackHelper internet;
	internet.Install (routerNodes);

	NetworkAddress   net;
	net.addr = "10.1.1.0"; net.mask = "255.255.255.0";
	SwitchConfig(switch1, lan1, net);

	net.addr = "10.1.2.0"; net.mask = "255.255.255.0";
	SwitchConfig(switch2, lan2, net);

	net.addr = "10.1.3.0"; net.mask = "255.255.255.0";
	PointToPointConfig(R1, R2, net);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	PrintRoutingTable (R1);
	PrintRoutingTable (R2);

	setUdpOnOffApp(n[1], m[1]);
	setUdpOnOffApp(m[2], n[2]);
	setMyTcpApp   (n[0], m[0]);
	setMyTcpApp   (R2  , m[3]);

	Simulator::Stop (Seconds(SIM_STOP));
	Simulator::Run ();

	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");
}
