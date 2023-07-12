/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Author: F.Qian
//    n1
//     |
//   --- (n0)|Router --- AP ------ STA               (waypoint)
//     |                            o------move to ------>o
//    n2
//  |- csma -|--- p2p ---|-- Wifi --|
//  Tcp flow: n1 -> STA ,Udp flow: n2 -> STA

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

using namespace ns3; 

#define TH_INTERVAL 	0.1
#define CW_INTERVAL     0.1

#define	PROG_DIR	"wlan/exp03/data/"

NS_LOG_COMPONENT_DEFINE ("exp03-intraMode");

class NetSim {
    public :
        NetSim();
        ~NetSim();

	void RunSim (int, char **);

    private :
        void Configure();
        void CreateNetworkTopology();
        void ConfigureDataLinkLayer(std::string, uint32_t);
        void ConfigureNetworkLayer ();
        void ConfigureAnimation();

	void SetUdpEchoApplication(Ptr<Node>, Ptr<Node>);
	void setTcpApplication(Ptr <Node>, Ptr <Node>);

        Ptr<Node>    *n; // nodes in csma network segment
        Ptr<Node>    mn; // mobile STA node

	Ptr<Node> Router;
	Ptr<Node> AP;

	NodeContainer wifiAP;
	NodeContainer p2pNodes;
	NodeContainer csmaNodes;

	uint32_t oldTotalBytes;
	double   throughput;

	static void CwndTracer (Ptr<OutputStreamWrapper>, uint32_t, uint32_t);
	static void MyEventHandller (Ptr<Application>, Ptr<OutputStreamWrapper>);
	static void MyEventWrapper (Ptr<Application>, Ptr<OutputStreamWrapper>);
	void TraceThroughput (Ptr<Application>, Ptr<OutputStreamWrapper>);

	uint32_t nCsma; // node numbers in Csma net

	NetDeviceContainer staDevices;
	NetDeviceContainer p2pDevices;
	NetDeviceContainer csmaDevices;
	NetDeviceContainer apDevices;
};

NetSim::NetSim() : oldTotalBytes(0), throughput(0.0)
{
	nCsma = 2;
}

NetSim::~NetSim()
{
        for (uint32_t i = 0; i < nCsma; ++i) n[i] = 0;
        n=0; mn=0;
}


void // trace throughput in kbps
NetSim::TraceThroughput (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
	NS_LOG_FUNCTION(this);

	uint32_t newTotalBytes;

        Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
        newTotalBytes = pktSink->GetTotalRx ();
        double th = (newTotalBytes - oldTotalBytes)*8.0/TH_INTERVAL/1024;

        *stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << th << std::endl;
        throughput = (throughput+th)/2.0;
        oldTotalBytes = newTotalBytes;
        Simulator::Schedule (Seconds (TH_INTERVAL), &NetSim::TraceThroughput, this, app, stream);
}

void 
NetSim::CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
        // cwnd is traced in byte unit ??
        *stream->GetStream() << Simulator::Now ().GetSeconds () 
                << " \t " << newcwnd << std::endl;
}

void 
NetSim::MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Ptr<Socket> src_socket  = app->GetObject<OnOffApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, st));
}

void 
NetSim::MyEventWrapper (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Simulator::Schedule (Seconds (CW_INTERVAL), &MyEventHandller, app, st);
}

void // set UDP flow from n to m
NetSim::SetUdpEchoApplication(Ptr<Node> n, Ptr<Node> m)
{
	NS_LOG_FUNCTION(this);

	UdpEchoServerHelper echoServer (9);
	ApplicationContainer serverApps = echoServer.Install (n);
	serverApps.Start (Seconds (0.1));
	serverApps.Stop (Seconds (20.1));

	// get server's IP address
	Ipv4InterfaceAddress adr = n->GetObject <Ipv4> ()->GetAddress(1, 0);

	// set UDP Echo Clients
	UdpEchoClientHelper echoClient (Ipv4Address(adr.GetLocal()), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (2000));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (512));
	ApplicationContainer clientApps = echoClient.Install (m);
	clientApps.Start (Seconds (0.1));
	clientApps.Stop  (Seconds (20.1));
}

void // set TCP flow from n to m
NetSim::setTcpApplication(Ptr <Node> n, Ptr <Node> m)
{
	NS_LOG_FUNCTION(this);

        // get node m's IP address as remote site address
        Ipv4InterfaceAddress madr = m->GetObject <Ipv4> ()->GetAddress(1, 0);
        Address remoteAddr (InetSocketAddress (madr.GetLocal(), 8888));

        OnOffHelper ftp  ("ns3::TcpSocketFactory", Address());
        ftp.SetAttribute ("OnTime", 
		StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        ftp.SetAttribute ("OffTime", 
		StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        ftp.SetAttribute ("Remote", AddressValue(remoteAddr));
        ApplicationContainer src = ftp.Install (n);
	src.Start (Seconds (0.1));
        src.Stop  (Seconds (20.1));

        // Create an optional packet sink to receive these packets
        PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                Address (InetSocketAddress (Ipv4Address::GetAny (), 8888)));
        ApplicationContainer sink = sinkHelper.Install (m);
        sink.Start (Seconds (0.1));
        sink.Stop  (Seconds (20.1 ));

	AsciiTraceHelper ascii;
	std::string tf = std::string(PROG_DIR) + "exp03-throughput.tr";
	Ptr<OutputStreamWrapper> st = ascii.CreateFileStream(tf);
	Simulator::Schedule (Seconds (TH_INTERVAL+0.2), &NetSim::TraceThroughput, this, sink.Get(0), st);

	std::string cf = std::string(PROG_DIR) + "exp03-cwnd.tr";
	Ptr<OutputStreamWrapper> st2 = ascii.CreateFileStream(cf);
	Simulator::Schedule (Seconds (0.1), &MyEventWrapper, src.Get(0), st2);
}

void
NetSim::Configure()
{
	NS_LOG_FUNCTION(this);

        Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
        Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
	Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("5Mbps"));
}

void
NetSim::CreateNetworkTopology()
{
	NS_LOG_FUNCTION(this);

	// set WIFI nodes ---------------------------------------------------
	mn = CreateObject<Node> ();

	// set AP nodes -----------------------------------------------------
	AP = CreateObject<Node> ();
	wifiAP.Add (AP);

	// set P2P nodes ----------------------------------------------------
	Router = CreateObject<Node> ();
	p2pNodes.Add (wifiAP);
	p2pNodes.Add (Router);

	// set CSMA nodes ---------------------------------------------------
	csmaNodes.Add (p2pNodes.Get (1));
	n = new Ptr<Node> [nCsma];
	for (uint32_t i = 0; i < nCsma; ++i) {
                n[i] = CreateObject<Node> ();
		csmaNodes.Add (n[i]);
	}
}

void 
NetSim::ConfigureDataLinkLayer(std::string lossType, uint32_t distance)
{
	NS_LOG_FUNCTION(this);

	// set wifi device ---------------------------------------------------
	// wavelength(lambda) at 2.4 GHz is 0.125m
	// the reference loss L0 = 20log(4pi/lambda) = 40.045997
	// the loss L = L0 + 10*Exponent*log(d/d0) =100.045997
	// the energy threshold of a received signal is -96dBm
	// so, the minimum available transmission level (dbm) is L-96=4.045997(dbm)
	YansWifiChannelHelper channel;
	channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	if(lossType == "log" ) {
		channel.AddPropagationLoss  ("ns3::LogDistancePropagationLossModel",
			"Exponent", DoubleValue(3.0),
			"ReferenceDistance", DoubleValue(1.0),
			"ReferenceLoss"    , DoubleValue(40.045997));
	} else if (lossType == "range" ) {
		channel.AddPropagationLoss  ("ns3::RangePropagationLossModel", 
		"MaxRange", DoubleValue(100.0));
	}

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.Set ("TxPowerStart", DoubleValue (4.045997) ); // for 802.11b, 100m
	phy.Set ("TxPowerEnd"  , DoubleValue (4.045997) ); // for 802.11b, 100m
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		"DataMode"   , StringValue ("DsssRate1Mbps"),  // for 802.11b
		"ControlMode", StringValue ("DsssRate1Mbps")); // for 802.11b

	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
	Ssid ssid = Ssid ("exp03-SSID");
	mac.SetType ("ns3::StaWifiMac", 
		"Ssid", SsidValue (ssid),
		"ActiveProbing", BooleanValue (false));
	staDevices = wifi.Install (phy, mac, mn);

	mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
	apDevices = wifi.Install (phy, mac, wifiAP);

	// set p2p device ---------------------------------------------------
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));
	p2pDevices = pointToPoint.Install (p2pNodes);

	// set csma device --------------------------------------------------
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay"   , TimeValue (NanoSeconds (6560)));
	csmaDevices = csma.Install (csmaNodes);

	// set mobility -----------------------------------------------------
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (110.0, 70, 0.0));
	mobility.Install (wifiAP);

	Ptr<WaypointMobilityModel> m_mob;
        m_mob = CreateObject<WaypointMobilityModel> ();
        mn->AggregateObject (m_mob);

	Waypoint waypoint0  (Seconds ( 0.1), Vector (160.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint0);
        Waypoint waypoint1  (Seconds ( 5.1), Vector (160.0+distance, 70, 0.0));
        m_mob->AddWaypoint  (waypoint1);
        Waypoint waypoint2  (Seconds (10.1), Vector (160.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint2);
        Waypoint waypoint3  (Seconds (15.1), Vector (160.0+distance, 70, 0.0));
        m_mob->AddWaypoint  (waypoint3);
        Waypoint waypoint4  (Seconds (20.1), Vector (160.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint4);
}

void 
NetSim::ConfigureNetworkLayer ()
{
	NS_LOG_FUNCTION(this);

	// Install internet stack -------------------------------------------
	InternetStackHelper stack;
	stack.InstallAll ();

	// Install Ipv4 addresses -------------------------------------------
	Ipv4AddressHelper address;

	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer csmaInterfaces;
	csmaInterfaces = address.Assign (csmaDevices);

	address.SetBase ("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterfaces;
	p2pInterfaces = address.Assign (p2pDevices);

	address.SetBase ("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer apInterface;
	apInterface = address.Assign (apDevices);

	Ipv4InterfaceContainer staInterfaces;
	staInterfaces = address.Assign (staDevices);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

void
NetSim::ConfigureAnimation()
{
	NS_LOG_FUNCTION(this);

	AnimationInterface::SetConstantPosition (n[0], 10, 40); 
	AnimationInterface::SetConstantPosition (n[1], 10, 100); 
	AnimationInterface::SetConstantPosition (Router, 60, 70); 
	AnimationInterface::SetConstantPosition (AP, 110, 70); 

/*
	AnimationInterface::SetNodeDescription (AP, "AP");
	AnimationInterface::SetNodeDescription (mn, "STA"); 
	AnimationInterface::SetNodeDescription (csmaNodes, "CSMA");
	AnimationInterface::SetNodeDescription (Router, "Router");
	AnimationInterface::SetNodeColor (wifiAP, 0, 255, 0);
	AnimationInterface::SetNodeColor (mn, 255, 0, 0);
	AnimationInterface::SetNodeColor (csmaNodes, 0, 0, 255);
*/
}

void
NetSim::RunSim(int argc, char *argv[])
{
	NS_LOG_FUNCTION(this);

	std::string lossType = "log"; // default type is Log-Distance Propagation Loss Model
	uint32_t    distance = 80;    // default moving distance is 80m

	CommandLine cmd;
	cmd.AddValue ("lossType", "Type of propagation loss model(log|range)", lossType);
	cmd.AddValue ("distance", "moving distance from initial position(in m)", distance);
	cmd.Parse (argc,argv);

	Configure ();
	CreateNetworkTopology ();
	ConfigureDataLinkLayer(lossType, distance);
	ConfigureNetworkLayer ();

	// Install applications ---------------------------------------------
	SetUdpEchoApplication(n[0], mn);
	setTcpApplication(n[1], mn);

	ConfigureAnimation();
	std::string xf = std::string(PROG_DIR) + "exp03-infraTCP.xml";
	AnimationInterface anim (xf);
	anim.EnablePacketMetadata (true);

	Simulator::Stop (Seconds (20.2));
	Simulator::Run ();
	Simulator::Destroy ();
}

int 
main (int argc, char *argv[])
{
	NetSim sim;
	sim.RunSim(argc, argv);
	return 0;
}

