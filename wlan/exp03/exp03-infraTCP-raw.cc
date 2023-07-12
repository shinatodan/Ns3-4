/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Author: F.Qian
//
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
#define CW_INTERVAL     0.01

#define	PROG_DIR	"wlan/exp03/"

NS_LOG_COMPONENT_DEFINE ("exp03");

uint32_t oldTotalBytes=0;
uint32_t newTotalBytes;
double   throughput=0.0;

void // trace throughput in kbps
TraceThroughput (Ptr<Application> app, Ptr<OutputStreamWrapper> stream)
{
        Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
        newTotalBytes = pktSink->GetTotalRx ();
        double th = (newTotalBytes - oldTotalBytes)*8.0/TH_INTERVAL/1024;
        *stream->GetStream() << Simulator::Now ().GetSeconds () << " \t " << th << std::endl;
        throughput = (throughput+th)/2.0;
        oldTotalBytes = newTotalBytes;
        Simulator::Schedule (Seconds (TH_INTERVAL), &TraceThroughput, app, stream);
}

void 
CwndTracer (Ptr<OutputStreamWrapper>stream, uint32_t oldcwnd, uint32_t newcwnd)
{
        // cwnd is traced in byte unit ??
        *stream->GetStream() << Simulator::Now ().GetSeconds () 
                << " \t " << newcwnd << std::endl;
}

void 
MyEventHandller (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Ptr<Socket> src_socket  = app->GetObject<OnOffApplication>()->GetSocket();
        src_socket->TraceConnectWithoutContext("CongestionWindow", 
                MakeBoundCallback(&CwndTracer, st));
}

void 
MyEventWrapper (Ptr<Application> app, Ptr<OutputStreamWrapper> st)
{
        Simulator::Schedule (Seconds (CW_INTERVAL), &MyEventHandller, app, st);
}

void // set UDP flow from n to m
SetUdpEchoApplication(Ptr<Node> n, Ptr<Node> m)
{
	UdpEchoServerHelper echoServer (9);
	ApplicationContainer serverApps = echoServer.Install (n);
	serverApps.Start (Seconds (2.0));
	serverApps.Stop (Seconds (20.0));

	// get server's IP address
	Ipv4InterfaceAddress adr = n->GetObject <Ipv4> ()->GetAddress(1, 0);

	// set UDP Echo Clients
	UdpEchoClientHelper echoClient (Ipv4Address(adr.GetLocal()), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (2000));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.1)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (512));
	ApplicationContainer clientApps = echoClient.Install (m);
	clientApps.Start (Seconds (2.0));
	clientApps.Stop  (Seconds (20.0));
}

void // set TCP flow from n to m
setTcpApplication(Ptr <Node> n, Ptr <Node> m)
{
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
	std::string tf = std::string(PROG_DIR) + "data/exp03-throughput.tr";
	Ptr<OutputStreamWrapper> st = ascii.CreateFileStream(tf);
	Simulator::Schedule (Seconds (TH_INTERVAL+0.2), &TraceThroughput, sink.Get(0), st);

	std::string cf = std::string(PROG_DIR) + "data/exp03-cwnd.tr";
	Ptr<OutputStreamWrapper> st2 = ascii.CreateFileStream(cf);
	Simulator::Schedule (Seconds (0.1), &MyEventWrapper, src.Get(0), st2);
}

int 
main (int argc, char *argv[])
{
	CommandLine cmd;
	cmd.Parse (argc,argv);

	Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (1024));
	Config::SetDefault ("ns3::OnOffApplication::DataRate"  , StringValue ("3Mbps"));

	NodeContainer allNodes;

	// set WIFI nodes ---------------------------------------------------
	Ptr<Node> mn = CreateObject<Node> ();
	allNodes.Add (mn);

	YansWifiChannelHelper channel;
	channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
        //channel.AddPropagationLoss  ("ns3::FriisPropagationLossModel");
	channel.AddPropagationLoss  ("ns3::LogDistancePropagationLossModel",
                "Exponent", DoubleValue(3.0),
                "ReferenceDistance", DoubleValue(1.0),
                "ReferenceLoss", DoubleValue(46.6777));

	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	//phy.Set ("RxGain", DoubleValue (-10) ); //
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		"DataMode"   , StringValue ("DsssRate1Mbps"),
		"ControlMode", StringValue ("DsssRate1Mbps"));

	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

	Ssid ssid = Ssid ("exp03-SSID");
	mac.SetType ("ns3::StaWifiMac", 
		"Ssid", SsidValue (ssid),
		"ActiveProbing", BooleanValue (false));

	NetDeviceContainer staDevices;
	staDevices = wifi.Install (phy, mac, mn);

	// set AP nodes -----------------------------------------------------
	NodeContainer wifiAP;
	Ptr<Node> AP = CreateObject<Node> ();
	wifiAP.Add (AP);
	allNodes.Add (AP);

	mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
	NetDeviceContainer apDevices;
	apDevices = wifi.Install (phy, mac, wifiAP);

	// set P2P nodes ----------------------------------------------------
	Ptr<Node> Router = CreateObject<Node> ();
	NodeContainer p2pNodes;
	p2pNodes.Add (wifiAP);
	p2pNodes.Add (Router);
	allNodes.Add (Router);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (10));

	NetDeviceContainer p2pDevices;
	p2pDevices = pointToPoint.Install (p2pNodes);

	// set CSMA nodes ---------------------------------------------------
	uint32_t nCsma = 2;
	NodeContainer csmaNodes;
	csmaNodes.Add (p2pNodes.Get (1));
	Ptr<Node> *n = new Ptr<Node> [nCsma];
	for (uint32_t i = 0; i < nCsma; ++i) {
                n[i] = CreateObject<Node> ();
		csmaNodes.Add (n[i]);
		allNodes.Add  (n[i]);
	}

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay"   , TimeValue (NanoSeconds (6560)));
	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);

	// set mobility -----------------------------------------------------
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiAP);

	Ptr<WaypointMobilityModel> m_mob;
        m_mob = CreateObject<WaypointMobilityModel> ();
        mn->AggregateObject (m_mob);

	Waypoint waypoint0  (Seconds (0.0), Vector (160.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint0);
        Waypoint waypoint1  (Seconds (10.0), Vector (300.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint1);
        Waypoint waypoint2  (Seconds (20.0), Vector (160.0, 70, 0.0));
        m_mob->AddWaypoint  (waypoint2);

	AnimationInterface::SetConstantPosition (n[0], 10, 40); 
	AnimationInterface::SetConstantPosition (n[1], 10, 100); 
	AnimationInterface::SetConstantPosition (Router, 60, 70); 
	AnimationInterface::SetConstantPosition (AP, 110, 70); 

	// Install internet stack -------------------------------------------
	InternetStackHelper stack;
	stack.Install (allNodes);

	// Install Ipv4 addresses -------------------------------------------
	Ipv4AddressHelper address;

	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer csmaInterfaces;
	csmaInterfaces = address.Assign (csmaDevices);

	address.SetBase ("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInterfaces;
	p2pInterfaces = address.Assign (p2pDevices);

	address.SetBase ("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer staInterfaces;
	staInterfaces = address.Assign (staDevices);

	Ipv4InterfaceContainer apInterface;
	apInterface = address.Assign (apDevices);
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Install applications ---------------------------------------------
	SetUdpEchoApplication(n[0], mn);
	setTcpApplication(n[1], mn);

	AnimationInterface::SetNodeDescription (wifiAP, "AP");
	AnimationInterface::SetNodeDescription (mn, "STA"); 
	AnimationInterface::SetNodeDescription (csmaNodes, "CSMA");
	AnimationInterface::SetNodeColor (wifiAP, 0, 255, 0);
	AnimationInterface::SetNodeColor (mn, 255, 0, 0);
	AnimationInterface::SetNodeColor (csmaNodes, 0, 0, 255);

	std::string xf = std::string(PROG_DIR) + "exp03-infraTCP.xml";
	AnimationInterface anim (xf);
	anim.EnablePacketMetadata (true);

	Simulator::Stop (Seconds (20.2));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}

